/**
 * @file Transition.h
 * @brief 상태 간 전이를 정의하는 클래스
 *
 * 하나의 상태에서 다른 상태로의 전이 조건과 인터락을 관리합니다.
 * 전이 평가 순서: guard 조건 체크 -> 인터락 체크 -> 전이 실행
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace YggdraSeq
{

// 전방 선언
class Interlock;

/**
 * @class Transition
 * @brief 상태 간 전이를 정의하는 클래스
 *
 * 출발 상태(fromState)에서 도착 상태(toState)로의 전이 조건을 캡슐화합니다.
 * - guard: 전이 가능 여부를 판단하는 조건 함수 (true일 때 전이 허용)
 * - interlocks: 전이 전 체크해야 할 안전 조건 목록
 * - priority: 동일 상태에서 복수 전이가 가능할 때 우선순위
 *
 * 평가 순서:
 * 1. guard 조건 체크 (guard가 없으면 true로 간주)
 * 2. 모든 인터락 체크 (하나라도 실패하면 전이 차단)
 * 3. 두 조건 모두 통과하면 전이 허용
 *
 * @note 인터락 체크를 건너뛰는 코드 경로는 절대 만들지 마세요.
 */
class AFX_EXT_CLASS Transition
{
public:
    /**
     * @brief Transition 생성자
     *
     * @param fromState 출발 상태 이름. IState::getName()과 일치해야 합니다.
     * @param toState   도착 상태 이름. SequenceEngine에 등록된 상태여야 합니다.
     * @param priority  우선순위. 값이 높을수록 먼저 평가됩니다. 기본값 0.
     */
    Transition(const std::string& fromState,
               const std::string& toState,
               int priority = 0);

    /**
     * @brief 소멸자
     */
    ~Transition();

    // ========================================================================
    // 전이 조건 설정
    // ========================================================================

    /**
     * @brief guard 조건 함수를 설정
     *
     * guard는 전이 가능 여부를 판단하는 사용자 정의 함수입니다.
     * true를 반환하면 전이 조건이 충족된 것이며, false면 전이하지 않습니다.
     * guard가 설정되지 않으면 항상 true로 간주합니다.
     *
     * @param guardFunc 전이 조건 함수. bool()을 반환하는 callable.
     *
     * @code
     * transition->setGuard([&]() { return alignComplete; });
     * @endcode
     */
    void setGuard(std::function<bool()> guardFunc);

    /**
     * @brief 인터락을 전이에 바인딩
     *
     * 전이 실행 전에 반드시 체크해야 할 인터락을 추가합니다.
     * 복수의 인터락을 추가할 수 있으며, 모두 통과해야 전이가 허용됩니다.
     *
     * @param interlock 바인딩할 인터락 (raw pointer 관찰용, 소유권 없음)
     * @note Interlock의 수명은 Transition보다 길어야 합니다.
     *       InterlockManager가 소유하고, Transition은 관찰만 합니다.
     */
    void addInterlock(Interlock* interlock);

    // ========================================================================
    // 전이 평가
    // ========================================================================

    /**
     * @brief 전이 조건을 평가하여 전이 가능 여부를 반환
     *
     * 평가 순서:
     * 1. guard 조건 체크 (미설정 시 true)
     * 2. 모든 인터락 체크 (하나라도 false면 전이 차단)
     *
     * @param[out] guardResult      guard 조건 평가 결과
     * @param[out] interlockResults 각 인터락의 개별 체크 결과
     * @return true = 전이 가능 (guard + 모든 인터락 통과), false = 전이 불가
     */
    bool evaluate(bool& guardResult,
                  std::vector<InterlockCheckResult>& interlockResults) const;

    // ========================================================================
    // 접근자
    // ========================================================================

    /**
     * @brief 출발 상태 이름 반환
     * @return 출발 상태 이름
     */
    const std::string& getFromState() const;

    /**
     * @brief 도착 상태 이름 반환
     * @return 도착 상태 이름
     */
    const std::string& getToState() const;

    /**
     * @brief 우선순위 반환
     * @return 우선순위 값 (높을수록 먼저 평가)
     */
    int getPriority() const;

    /**
     * @brief 바인딩된 인터락 목록 반환
     * @return 인터락 포인터 목록에 대한 const 참조
     */
    const std::vector<Interlock*>& getInterlocks() const;

private:
    std::string m_fromState;                ///< 출발 상태 이름
    std::string m_toState;                  ///< 도착 상태 이름
    std::function<bool()> m_guard;          ///< 전이 조건 함수 (미설정 시 nullptr)
    std::vector<Interlock*> m_interlocks;   ///< 바인딩된 인터락 목록 (관찰용, 소유권 없음)
    int m_priority;                         ///< 우선순위 (높을수록 먼저 평가)
};

} // namespace YggdraSeq
