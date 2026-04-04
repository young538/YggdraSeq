/**
 * @file SimulatedHardware.h
 * @brief 가상 외관검사 하드웨어 시뮬레이터
 *
 * 실제 장비 없이 센서값과 장치 상태를 시뮬레이션합니다.
 * 각 State의 onExecute()에서 이 클래스의 값을 점진적으로 변경하고,
 * Observer를 통해 View에 통지합니다.
 *
 * 시뮬레이션 값:
 *   - 카메라 초점 (m_cameraFocus)
 *   - 조명 밝기 (m_lightIntensity)
 *   - 스테이지 XY/Theta (m_stageX, m_stageY, m_stageTheta)
 *   - 로봇 팔 XY (m_armX, m_armY)
 *   - 그리퍼 상태 (m_gripState)
 *   - 웨이퍼 카운터 (m_waferCount, m_goodCount, m_ngCount)
 *
 * 인터락 조건 함수에서 이 값을 읽어 안전 판정에 사용합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <cstdint>

namespace YggdraSeq
{

/**
 * @class SimulatedHardware
 * @brief 가상 하드웨어 시뮬레이터 (싱글톤 대신 명시적 인스턴스 사용)
 *
 * 스레드 안전: 모든 getter/setter는 mutex로 보호됩니다.
 * 복수의 시퀀스 스레드(Inspection, Handler)에서 동시 접근 가능합니다.
 */
class SimulatedHardware
{
public:
    /**
     * @brief SimulatedHardware 생성자
     *
     * 모든 센서값을 기본값으로 초기화합니다.
     */
    SimulatedHardware();
    ~SimulatedHardware();

    // 복사/이동 금지
    SimulatedHardware(const SimulatedHardware&) = delete;
    SimulatedHardware& operator=(const SimulatedHardware&) = delete;

    // ========================================================================
    // 카메라 관련
    // ========================================================================

    /** @brief 카메라 초점 거리 설정 (mm) */
    void setCameraFocus(double focus);
    /** @brief 카메라 초점 거리 반환 (mm) */
    double getCameraFocus() const;

    // ========================================================================
    // 조명 관련
    // ========================================================================

    /** @brief 조명 밝기 설정 (0~100%) */
    void setLightIntensity(double intensity);
    /** @brief 조명 밝기 반환 (%) */
    double getLightIntensity() const;

    // ========================================================================
    // 스테이지 관련
    // ========================================================================

    /** @brief 스테이지 X 위치 설정 (mm) */
    void setStageX(double x);
    /** @brief 스테이지 X 위치 반환 (mm) */
    double getStageX() const;

    /** @brief 스테이지 Y 위치 설정 (mm) */
    void setStageY(double y);
    /** @brief 스테이지 Y 위치 반환 (mm) */
    double getStageY() const;

    /** @brief 스테이지 회전 각도 설정 (degree) */
    void setStageTheta(double theta);
    /** @brief 스테이지 회전 각도 반환 (degree) */
    double getStageTheta() const;

    // ========================================================================
    // 로봇 팔 관련
    // ========================================================================

    /** @brief 로봇 팔 X 위치 설정 (mm) */
    void setArmX(double x);
    /** @brief 로봇 팔 X 위치 반환 (mm) */
    double getArmX() const;

    /** @brief 로봇 팔 Y 위치 설정 (mm) */
    void setArmY(double y);
    /** @brief 로봇 팔 Y 위치 반환 (mm) */
    double getArmY() const;

    /** @brief 그리퍼 파지 상태 설정 (true=파지, false=해제) */
    void setGripState(bool gripping);
    /** @brief 그리퍼 파지 상태 반환 */
    bool getGripState() const;

    // ========================================================================
    // 웨이퍼 카운터
    // ========================================================================

    /** @brief 카세트 잔여 웨이퍼 수 설정 */
    void setWaferCount(int count);
    /** @brief 카세트 잔여 웨이퍼 수 반환 */
    int getWaferCount() const;

    /** @brief Good 판정 카운트 증가 */
    void incrementGoodCount();
    /** @brief Good 판정 카운트 반환 */
    int getGoodCount() const;

    /** @brief NG 판정 카운트 증가 */
    void incrementNgCount();
    /** @brief NG 판정 카운트 반환 */
    int getNgCount() const;

    // ========================================================================
    // 전체 초기화
    // ========================================================================

    /**
     * @brief 모든 센서값을 기본값으로 초기화
     *
     * 시퀀스 재시작 시 호출합니다.
     */
    void reset();

    // ========================================================================
    // 인터락 조건 헬퍼
    // ========================================================================

    /**
     * @brief 로봇 팔이 검사 영역 밖에 있는지 확인
     *
     * 검사 영역: X < 200mm (카메라/스테이지 위치 근처)
     * @return true = 팔이 안전 영역(검사 영역 밖)에 있음
     */
    bool isArmOutOfInspectArea() const;

    /**
     * @brief 웨이퍼가 스테이지에 존재하는지 확인
     *
     * @return true = 웨이퍼가 감지됨 (waferCount > 0 상태에서 로드 완료)
     */
    bool isWaferPresent() const;

    /**
     * @brief 스테이지가 안정 위치에 있는지 확인
     *
     * @return true = 스테이지 X/Y/Theta가 목표값 근처
     */
    bool isStageStable() const;

    // ========================================================================
    // Observer 통지 (HWND 기반 PostMessage)
    // ========================================================================

    /**
     * @brief UI 통지용 HWND 설정
     *
     * 값 변경 시 WM_APP_HARDWARE_UPDATED를 PostMessage합니다.
     *
     * @param hWnd 통지를 받을 윈도우 핸들
     */
    void setNotifyWindow(HWND hWnd);

private:
    /**
     * @brief 값 변경 통지 전송
     */
    void notifyUpdate();

    // 센서값 (mutex로 보호)
    double m_cameraFocus;       ///< 카메라 초점 거리 (mm), 기본값: 0.0
    double m_lightIntensity;    ///< 조명 밝기 (%), 기본값: 0.0
    double m_stageX;            ///< 스테이지 X (mm), 기본값: 0.0
    double m_stageY;            ///< 스테이지 Y (mm), 기본값: 0.0
    double m_stageTheta;        ///< 스테이지 Theta (deg), 기본값: 0.0
    double m_armX;              ///< 로봇 팔 X (mm), 기본값: 300.0 (안전 위치)
    double m_armY;              ///< 로봇 팔 Y (mm), 기본값: 0.0
    bool m_gripState;           ///< 그리퍼 상태, 기본값: false (해제)
    int m_waferCount;           ///< 카세트 잔여 웨이퍼, 기본값: 25
    int m_goodCount;            ///< Good 카운트, 기본값: 0
    int m_ngCount;              ///< NG 카운트, 기본값: 0

    std::atomic<bool> m_waferOnStage;  ///< 웨이퍼가 스테이지 위에 있는지

    mutable std::mutex m_mutex;  ///< 스레드 동기화용

    HWND m_hNotifyWnd;          ///< UI 통지용 윈도우 핸들
};

} // namespace YggdraSeq
