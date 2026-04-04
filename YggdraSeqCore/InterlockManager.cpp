/**
 * @file InterlockManager.cpp
 * @brief InterlockManager 클래스 구현
 *
 * 전역 인터락의 등록, 제거, 일괄 체크 로직을 구현합니다.
 * EMO 인터락 위반 시 콜백 호출을 포함합니다.
 */

#include "pch.h"
#include "InterlockManager.h"
#include <algorithm>

namespace YggdraSeq
{

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

InterlockManager::InterlockManager()
    : m_emoCallback(nullptr)
{
}

InterlockManager::~InterlockManager() = default;

// ============================================================================
// 인터락 등록 / 제거
// ============================================================================

ErrorCode InterlockManager::addInterlock(std::unique_ptr<Interlock> interlock)
{
    if (!interlock)
    {
        return ErrorCode::UnknownError;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 같은 이름의 인터락이 이미 존재하는지 확인
    const std::string& name = interlock->getName();
    for (const auto& existing : m_globalInterlocks)
    {
        if (existing->getName() == name)
        {
            return ErrorCode::InterlockAlreadyRegistered;
        }
    }

    m_globalInterlocks.push_back(std::move(interlock));
    return ErrorCode::Success;
}

ErrorCode InterlockManager::removeInterlock(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_globalInterlocks.begin(), m_globalInterlocks.end(),
        [&name](const std::unique_ptr<Interlock>& interlock)
        {
            return interlock->getName() == name;
        });

    if (it == m_globalInterlocks.end())
    {
        return ErrorCode::InterlockNotFound;
    }

    m_globalInterlocks.erase(it);
    return ErrorCode::Success;
}

// ============================================================================
// 인터락 체크
// ============================================================================

bool InterlockManager::checkAll(std::vector<InterlockCheckResult>& results) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    results.clear();
    results.reserve(m_globalInterlocks.size());

    bool allPassed = true;

    for (const auto& interlock : m_globalInterlocks)
    {
        InterlockCheckResult result = interlock->check();

        if (!result.passed)
        {
            allPassed = false;

            // EMO 심각도 위반 시 콜백 호출
            if (result.severity == InterlockSeverity::EMO && m_emoCallback)
            {
                m_emoCallback(*interlock);
            }
        }

        results.push_back(std::move(result));
    }

    return allPassed;
}

// ============================================================================
// EMO 콜백 설정
// ============================================================================

void InterlockManager::setEmoCallback(std::function<void(const Interlock&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_emoCallback = std::move(callback);
}

// ============================================================================
// 접근자
// ============================================================================

Interlock* InterlockManager::findInterlock(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& interlock : m_globalInterlocks)
    {
        if (interlock->getName() == name)
        {
            return interlock.get();
        }
    }

    return nullptr;
}

size_t InterlockManager::getInterlockCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_globalInterlocks.size();
}

std::vector<std::string> InterlockManager::getInterlockNames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_globalInterlocks.size());

    for (const auto& interlock : m_globalInterlocks)
    {
        names.push_back(interlock->getName());
    }

    return names;
}

} // namespace YggdraSeq
