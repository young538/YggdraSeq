/**
 * @file Interlock.h
 * @brief 안전 조건 검증 클래스
 *
 * 센서 값, 밸브 상태 등 하드웨어 안전 조건을 체크하는 단위입니다.
 * 각 Interlock은 조건 함수(condition)를 가지며, check() 메서드로
 * 현재 안전 상태를 평가합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <functional>
#include <mutex>

namespace YggdraSeq
{

/**
 * @class Interlock
 * @brief 안전 조건을 검증하는 단위 클래스
 *
 * 반도체 장비의 안전 조건(인터락)을 캡슐화합니다.
 * condition 함수가 true를 반환하면 안전, false면 위반입니다.
 *
 * 심각도별 대응:
 * - Warning:  로그 기록만, 시퀀스 계속 진행
 * - Critical: 시퀀스 일시 정지, 운전자 개입 대기
 * - EMO:      모든 스레드 즉시 비상 정지, EMO 콜백 호출
 *
 * @note 모든 public 메서드는 스레드 안전합니다 (mutex 보호).
 */
class AFX_EXT_CLASS Interlock
{
public:
    /**
     * @brief Interlock 생성자
     *
     * @param name        인터락 고유 이름 (식별용)
     * @param condition   안전 조건 함수 (true = 안전, false = 위반)
     * @param severity    인터락 심각도 (Warning/Critical/EMO)
     * @param description 인터락 설명 (로깅/디스플레이용, 선택 사항)
     */
    Interlock(const std::string& name,
              std::function<bool()> condition,
              InterlockSeverity severity,
              const std::string& description = "");

    /**
     * @brief 소멸자
     */
    ~Interlock();

    // ========================================================================
    // 인터락 체크
    // ========================================================================

    /**
     * @brief 인터락 조건을 체크하고 결과를 반환
     *
     * condition 함수를 호출하여 현재 안전 상태를 평가합니다.
     * 체크 시간(lastCheckTime)을 갱신하고 결과를 InterlockCheckResult로 반환합니다.
     *
     * @return InterlockCheckResult 체크 결과 (이름, 통과 여부, 심각도, 현재값 등)
     * @note 스레드 안전합니다. 내부 mutex로 동기화됩니다.
     */
    InterlockCheckResult check() const;

    // ========================================================================
    // 현재값 / 임계값 관리
    // ========================================================================

    /**
     * @brief 현재 값을 설정 (모니터링 표시용)
     *
     * 인터락 체크와 별도로, 현재 센서/조건 값을 문자열로 저장합니다.
     * View에서 CInterlockPane의 "현재 값" 컬럼에 표시됩니다.
     *
     * @param value 현재 값 (문자열 형태, 예: "125.3mm")
     */
    void setCurrentValue(const std::string& value);

    /**
     * @brief 임계 값을 설정 (모니터링 표시용)
     *
     * 인터락 판정 기준값을 문자열로 저장합니다.
     * View에서 CInterlockPane의 "임계 값" 컬럼에 표시됩니다.
     *
     * @param value 임계 값 (문자열 형태, 예: "100.0mm")
     */
    void setThresholdValue(const std::string& value);

    // ========================================================================
    // 접근자
    // ========================================================================

    /**
     * @brief 인터락 이름 반환
     * @return 인터락 고유 이름
     */
    const std::string& getName() const;

    /**
     * @brief 인터락 심각도 반환
     * @return 심각도 (Warning/Critical/EMO)
     */
    InterlockSeverity getSeverity() const;

    /**
     * @brief 인터락 설명 반환
     * @return 인터락 설명 문자열
     */
    const std::string& getDescription() const;

    /**
     * @brief 현재 값 반환
     * @return 현재 값 문자열
     */
    std::string getCurrentValue() const;

    /**
     * @brief 임계 값 반환
     * @return 임계 값 문자열
     */
    std::string getThresholdValue() const;

    /**
     * @brief 마지막 체크 시간 반환
     * @return 마지막으로 check()가 호출된 시간
     */
    TimePoint getLastCheckTime() const;

private:
    std::string m_name;                     ///< 인터락 고유 이름
    std::function<bool()> m_condition;      ///< 안전 조건 함수 (true = 안전)
    InterlockSeverity m_severity;           ///< 심각도
    std::string m_description;              ///< 인터락 설명 (디스플레이용)

    mutable std::string m_currentValue;     ///< 현재 값 (모니터링 표시용)
    std::string m_thresholdValue;           ///< 임계 값 (모니터링 표시용)
    mutable TimePoint m_lastCheckTime;      ///< 마지막 체크 시간

    mutable std::mutex m_mutex;             ///< 스레드 동기화용 뮤텍스
};

} // namespace YggdraSeq
