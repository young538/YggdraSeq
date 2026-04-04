/**
 * @file Transition.cpp
 * @brief Transition 클래스 구현
 *
 * 상태 간 전이 조건(guard + interlock)을 평가하는 로직을 구현합니다.
 * 평가 순서: guard 먼저 -> interlock 나중 (PRD 규칙 준수)
 */

#include "pch.h"
#include "Transition.h"
#include "Interlock.h"

namespace YggdraSeq
{

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

Transition::Transition(const std::string& fromState,
                       const std::string& toState,
                       int priority)
    : m_fromState(fromState)
    , m_toState(toState)
    , m_guard(nullptr)
    , m_priority(priority)
{
}

Transition::~Transition() = default;

// ============================================================================
// 전이 조건 설정
// ============================================================================

void Transition::setGuard(std::function<bool()> guardFunc)
{
    m_guard = std::move(guardFunc);
}

void Transition::addInterlock(Interlock* interlock)
{
    if (interlock != nullptr)
    {
        m_interlocks.push_back(interlock);
    }
}

// ============================================================================
// 전이 평가
// ============================================================================

bool Transition::evaluate(bool& guardResult,
                          std::vector<InterlockCheckResult>& interlockResults) const
{
    interlockResults.clear();

    // 1단계: guard 조건 체크 (미설정 시 true로 간주)
    if (m_guard)
    {
        guardResult = m_guard();
    }
    else
    {
        guardResult = true;
    }

    // guard가 false면 인터락 체크 없이 전이 차단
    if (!guardResult)
    {
        return false;
    }

    // 2단계: 모든 인터락 체크
    bool allInterlocksPassed = true;
    for (const auto* interlock : m_interlocks)
    {
        if (interlock != nullptr)
        {
            InterlockCheckResult result = interlock->check();
            if (!result.passed)
            {
                allInterlocksPassed = false;
            }
            interlockResults.push_back(std::move(result));
        }
    }

    // guard 통과 + 모든 인터락 통과 시에만 전이 허용
    return allInterlocksPassed;
}

// ============================================================================
// 접근자
// ============================================================================

const std::string& Transition::getFromState() const
{
    return m_fromState;
}

const std::string& Transition::getToState() const
{
    return m_toState;
}

int Transition::getPriority() const
{
    return m_priority;
}

const std::vector<Interlock*>& Transition::getInterlocks() const
{
    return m_interlocks;
}

} // namespace YggdraSeq
