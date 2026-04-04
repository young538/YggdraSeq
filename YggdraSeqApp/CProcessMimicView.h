/**
 * @file CProcessMimicView.h
 * @brief 외관검사 장비 미믹 다이어그램 뷰
 *
 * 반도체 외관검사 장비의 공정을 시각적으로 표현합니다.
 * CWnd에서 파생하며, Owner Draw로 장치들을 실시간 애니메이션합니다.
 *
 * 장치 구성:
 *   [Inspection 영역]
 *   - Loader (LDR): 카세트 + 웨이퍼 잔량
 *   - Stage (STG): XY/Theta 위치
 *   - Camera (CAM): 촬영 + Focus
 *   - Light (LED): 밝기 바
 *   - 검사 결과: Good/NG 카운터
 *
 *   [Handler 영역]
 *   - Robot Arm (ARM): XY 위치 + 그리퍼
 *   - Input Cassette (IN): 투입 카세트
 *   - Output Cassette (OUT): Good/NG 분류
 *
 * 시각 효과:
 *   - 실행 중 스텝의 장치 깜박임(pulse)
 *   - 인터락 위반 시 빨간 테두리
 *   - 웨이퍼 이동 애니메이션
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include <string>
#include <chrono>
#include <atomic>

namespace YggdraSeq
{

// 전방 선언
class SimulatedHardware;

/**
 * @class CProcessMimicView
 * @brief 외관검사 장비 미믹 다이어그램 (CWnd 파생, Owner Draw)
 *
 * 타이머(50ms)로 주기적으로 다시 그려 애니메이션 효과를 제공합니다.
 * SimulatedHardware의 값을 읽어 장치 위치와 상태를 실시간 표시합니다.
 */
class CProcessMimicView : public CWnd
{
    DECLARE_DYNAMIC(CProcessMimicView)

public:
    CProcessMimicView();
    virtual ~CProcessMimicView();

    /**
     * @brief 미믹 뷰 생성
     *
     * @param pParentWnd 부모 윈도우
     * @param rect       초기 영역
     * @return TRUE = 성공
     */
    BOOL CreateView(CWnd* pParentWnd, const CRect& rect);

    /**
     * @brief SimulatedHardware 참조 설정
     * @param pHardware 시뮬레이션 하드웨어 (관찰용, 소유권 없음)
     */
    void setHardware(SimulatedHardware* pHardware);

    /**
     * @brief 현재 활성 상태 이름 설정 (깜박임 효과용)
     * @param inspectionState 검사 시퀀스 현재 상태
     * @param handlerState    핸들러 시퀀스 현재 상태
     */
    void setActiveStates(const std::string& inspectionState, const std::string& handlerState);

    /**
     * @brief 인터락 위반 장치 설정 (빨간 테두리)
     * @param deviceName 위반된 장치 이름 (빈 문자열이면 해제)
     */
    void setInterlockViolation(const std::string& deviceName);

protected:
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    // ========================================================================
    // 그리기 메서드
    // ========================================================================

    /** @brief 전체 미믹 다이어그램 그리기 */
    void drawMimic(CDC* pDC, const CRect& clientRect);

    /** @brief Inspection 영역 그리기 */
    void drawInspectionArea(CDC* pDC, const CRect& areaRect);

    /** @brief Handler 영역 그리기 */
    void drawHandlerArea(CDC* pDC, const CRect& areaRect);

    /** @brief 장치 박스 그리기 (이름 + 값 표시) */
    void drawDevice(CDC* pDC, const CRect& rect, const CString& name,
        const CString& value, COLORREF color, bool active, bool violation);

    /** @brief Good/NG 카운터 그리기 */
    void drawCounter(CDC* pDC, const CRect& rect);

    /** @brief 연결선 그리기 (장치 간 화살표) */
    void drawConnection(CDC* pDC, CPoint from, CPoint to, bool active);

    /** @brief 웨이퍼 아이콘 그리기 */
    void drawWafer(CDC* pDC, CPoint pos, bool moving);

    // ========================================================================
    // 멤버 변수
    // ========================================================================

    SimulatedHardware* m_pHardware;     ///< 시뮬레이션 하드웨어 (관찰용)
    std::string m_inspectionState;      ///< 검사 시퀀스 현재 상태
    std::string m_handlerState;         ///< 핸들러 시퀀스 현재 상태
    std::string m_violationDevice;      ///< 인터락 위반 장치

    int m_animationFrame;               ///< 애니메이션 프레임 카운터 (깜박임용)
    bool m_pulseState;                  ///< 깜박임 on/off 상태
};

} // namespace YggdraSeq
