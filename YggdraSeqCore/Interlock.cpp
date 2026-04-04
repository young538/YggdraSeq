/**
 * @file Interlock.cpp
 * @brief Interlock 클래스 구현
 *
 * 안전 조건 체크 로직과 현재값/임계값 관리를 구현합니다.
 * 모든 public 메서드는 mutex로 스레드 안전하게 보호됩니다.
 */

#include "pch.h"
#include "Interlock.h"

namespace YggdraSeq
{

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

Interlock::Interlock(const std::string& name,
                     std::function<bool()> condition,
                     InterlockSeverity severity,
                     const std::string& description)
    : m_name(name)
    , m_condition(std::move(condition))
    , m_severity(severity)
    , m_description(description)
    , m_lastCheckTime(Clock::now())
{
}

Interlock::~Interlock() = default;

// ============================================================================
// 인터락 체크
// ============================================================================

InterlockCheckResult Interlock::check() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    InterlockCheckResult result;
    result.interlockName = m_name;
    result.severity = m_severity;
    result.description = m_description;
    result.currentValue = m_currentValue;
    result.thresholdValue = m_thresholdValue;

    // 조건 함수 실행 (true = 안전, false = 위반)
    if (m_condition)
    {
        result.passed = m_condition();
    }
    else
    {
        // 조건 함수가 없으면 안전으로 간주
        result.passed = true;
    }

    // 마지막 체크 시간 갱신
    m_lastCheckTime = Clock::now();

    return result;
}

// ============================================================================
// 현재값 / 임계값 관리
// ============================================================================

void Interlock::setCurrentValue(const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentValue = value;
}

void Interlock::setThresholdValue(const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_thresholdValue = value;
}

// ============================================================================
// 접근자
// ============================================================================

const std::string& Interlock::getName() const
{
    return m_name;
}

InterlockSeverity Interlock::getSeverity() const
{
    return m_severity;
}

const std::string& Interlock::getDescription() const
{
    return m_description;
}

std::string Interlock::getCurrentValue() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentValue;
}

std::string Interlock::getThresholdValue() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_thresholdValue;
}

TimePoint Interlock::getLastCheckTime() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastCheckTime;
}

} // namespace YggdraSeq
