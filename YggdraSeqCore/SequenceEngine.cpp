/**
 * @file SequenceEngine.cpp
 * @brief SequenceEngine 클래스 구현
 *
 * 시퀀스 실행의 핵심 로직을 구현합니다.
 * 워커 스레드에서 상태 머신을 구동하고, 전이를 평가하며,
 * 인터락을 체크하고, Observer에 이벤트를 통지합니다.
 *
 * 스레드 동기화:
 * - m_engineMutex: 엔진 상태 및 상태 맵 보호
 * - m_observerMutex: Observer 목록 보호
 * - m_pauseMutex + m_pauseCondition: pause/resume 동기화
 * - m_stepMutex + m_stepCondition: 디버그 스텝 실행 동기화
 * - m_debugMutex: 브레이크포인트 설정 보호
 * - atomic 변수: 플래그 값의 lock-free 접근
 */

#include "pch.h"
#include "SequenceEngine.h"
#include "IState.h"
#include "Transition.h"
#include "InterlockManager.h"
#include "ISequenceObserver.h"
#include "Logger.h"
#include <algorithm>

namespace YggdraSeq
{

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

SequenceEngine::SequenceEngine(const std::string& engineName)
    : m_engineName(engineName)
    , m_currentState(nullptr)
    , m_status(EngineStatus::Idle)
    , m_abortRequested(false)
    , m_pauseRequested(false)
    , m_interlockManager(std::make_unique<InterlockManager>())
    , m_debugMode(false)
    , m_stepRequested(false)
    , m_logger(std::make_shared<Logger>(engineName))
{
    // EMO 콜백 설정: EMO 인터락 위반 시 엔진을 즉시 중단
    m_interlockManager->setEmoCallback(
        [this](const Interlock& interlock)
        {
            m_logger->critical("InterlockManager",
                "EMO triggered: " + interlock.getName() + " - " + interlock.getDescription());

            // 엔진을 즉시 Error 상태로 전환하고 abort 요청
            m_abortRequested.store(true);
            setStatus(EngineStatus::Error);
        });

    m_logger->info("SequenceEngine", "Engine created: " + engineName);
}

SequenceEngine::~SequenceEngine()
{
    // 워커 스레드가 실행 중이면 안전하게 종료
    if (m_workerThread && m_workerThread->joinable())
    {
        m_abortRequested.store(true);

        // pause 상태에서 대기 중일 수 있으므로 깨움
        m_pauseRequested.store(false);
        m_pauseCondition.notify_all();
        m_stepCondition.notify_all();

        m_workerThread->join();
    }

    m_logger->info("SequenceEngine", "Engine destroyed: " + m_engineName);
}

// ============================================================================
// 시퀀스 실행 제어
// ============================================================================

ErrorCode SequenceEngine::run()
{
    std::lock_guard<std::mutex> lock(m_engineMutex);

    // 이미 실행 중인지 확인
    EngineStatus currentStatus = m_status.load();
    if (currentStatus == EngineStatus::Running || currentStatus == EngineStatus::Paused)
    {
        m_logger->warn("SequenceEngine", "run() called but engine already running");
        return ErrorCode::EngineAlreadyRunning;
    }

    // 등록된 상태가 있는지 확인
    if (m_states.empty())
    {
        m_logger->error("SequenceEngine", "run() called but no states registered");
        return ErrorCode::EngineNoStatesRegistered;
    }

    // 초기 상태가 설정되었는지 확인
    if (m_initialStateName.empty())
    {
        m_logger->error("SequenceEngine", "run() called but no initial state set");
        return ErrorCode::EngineNoInitialState;
    }

    // 초기 상태가 존재하는지 확인
    auto it = m_states.find(m_initialStateName);
    if (it == m_states.end())
    {
        m_logger->error("SequenceEngine", "Initial state not found: " + m_initialStateName);
        return ErrorCode::StateNotFound;
    }

    // 엔진 상태 초기화
    m_abortRequested.store(false);
    m_pauseRequested.store(false);
    m_stepRequested.store(false);
    m_currentState = it->second.get();

    // Logger에 Observer 목록 전달
    {
        std::lock_guard<std::mutex> obsLock(m_observerMutex);
        m_logger->setObservers(m_observers);
    }

    // 이전 워커 스레드가 남아있으면 정리
    if (m_workerThread && m_workerThread->joinable())
    {
        m_workerThread->join();
    }

    // 워커 스레드 시작
    m_workerThread = std::make_unique<std::thread>(&SequenceEngine::engineLoop, this);

    setStatus(EngineStatus::Running);
    m_logger->info("SequenceEngine", "Sequence started. Initial state: " + m_initialStateName);

    return ErrorCode::Success;
}

ErrorCode SequenceEngine::pause()
{
    EngineStatus currentStatus = m_status.load();
    if (currentStatus != EngineStatus::Running)
    {
        m_logger->warn("SequenceEngine", "pause() called but engine not running");
        return ErrorCode::EngineNotRunning;
    }

    m_pauseRequested.store(true);
    m_logger->info("SequenceEngine", "Pause requested");

    return ErrorCode::Success;
}

ErrorCode SequenceEngine::resume()
{
    EngineStatus currentStatus = m_status.load();
    if (currentStatus != EngineStatus::Paused)
    {
        m_logger->warn("SequenceEngine", "resume() called but engine not paused");
        return ErrorCode::EngineNotRunning;
    }

    m_pauseRequested.store(false);
    m_pauseCondition.notify_all();

    setStatus(EngineStatus::Running);
    m_logger->info("SequenceEngine", "Sequence resumed");

    return ErrorCode::Success;
}

ErrorCode SequenceEngine::abort()
{
    EngineStatus currentStatus = m_status.load();
    if (currentStatus == EngineStatus::Idle ||
        currentStatus == EngineStatus::Aborted ||
        currentStatus == EngineStatus::Complete)
    {
        return ErrorCode::EngineNotRunning;
    }

    m_logger->info("SequenceEngine", "Abort requested");
    m_abortRequested.store(true);

    // pause 상태에서 대기 중이면 깨움
    m_pauseRequested.store(false);
    m_pauseCondition.notify_all();
    m_stepCondition.notify_all();

    // 워커 스레드 종료 대기
    if (m_workerThread && m_workerThread->joinable())
    {
        m_workerThread->join();
    }

    // 현재 상태의 onExit()를 호출하여 리소스 정리
    if (m_currentState != nullptr)
    {
        m_currentState->onExit();
        m_currentState = nullptr;
    }

    setStatus(EngineStatus::Aborted);
    m_logger->info("SequenceEngine", "Sequence aborted");

    return ErrorCode::Success;
}

// ============================================================================
// 상태 등록
// ============================================================================

ErrorCode SequenceEngine::registerState(std::unique_ptr<IState> state)
{
    if (!state)
    {
        return ErrorCode::UnknownError;
    }

    std::lock_guard<std::mutex> lock(m_engineMutex);

    const std::string& name = state->getName();

    // 중복 확인
    if (m_states.find(name) != m_states.end())
    {
        m_logger->warn("SequenceEngine", "State already registered: " + name);
        return ErrorCode::StateAlreadyRegistered;
    }

    // 최대 상태 수 제한
    if (static_cast<int>(m_states.size()) >= Constants::MAX_STATE_COUNT)
    {
        m_logger->error("SequenceEngine", "Max state count reached");
        return ErrorCode::UnknownError;
    }

    m_logger->debug("SequenceEngine", "State registered: " + name);
    m_states[name] = std::move(state);

    return ErrorCode::Success;
}

ErrorCode SequenceEngine::setInitialState(const std::string& stateName)
{
    std::lock_guard<std::mutex> lock(m_engineMutex);

    if (m_states.find(stateName) == m_states.end())
    {
        m_logger->error("SequenceEngine", "Initial state not found: " + stateName);
        return ErrorCode::StateNotFound;
    }

    m_initialStateName = stateName;
    m_logger->info("SequenceEngine", "Initial state set: " + stateName);

    return ErrorCode::Success;
}

// ============================================================================
// 레시피 로드 (Phase 2 준비)
// ============================================================================

ErrorCode SequenceEngine::loadRecipe(const std::string& recipePath)
{
    // Phase 2에서 구현 예정
    m_logger->info("SequenceEngine", "loadRecipe() called (Phase 2 - not yet implemented): " + recipePath);
    return ErrorCode::Success;
}

// ============================================================================
// Observer 관리
// ============================================================================

void SequenceEngine::addObserver(ISequenceObserver* observer)
{
    if (observer == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_observerMutex);

    // 중복 등록 방지
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != m_observers.end())
    {
        return;
    }

    // 최대 Observer 수 제한
    if (static_cast<int>(m_observers.size()) >= Constants::MAX_OBSERVER_COUNT)
    {
        m_logger->warn("SequenceEngine", "Max observer count reached");
        return;
    }

    m_observers.push_back(observer);
    m_logger->setObservers(m_observers);
    m_logger->debug("SequenceEngine", "Observer added");
}

void SequenceEngine::removeObserver(ISequenceObserver* observer)
{
    std::lock_guard<std::mutex> lock(m_observerMutex);

    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != m_observers.end())
    {
        m_observers.erase(it);
        m_logger->setObservers(m_observers);
        m_logger->debug("SequenceEngine", "Observer removed");
    }
}

// ============================================================================
// 인터락 매니저 접근
// ============================================================================

InterlockManager& SequenceEngine::getInterlockManager()
{
    return *m_interlockManager;
}

const InterlockManager& SequenceEngine::getInterlockManager() const
{
    return *m_interlockManager;
}

// ============================================================================
// 디버그 모드
// ============================================================================

void SequenceEngine::setDebugMode(bool enabled)
{
    m_debugMode.store(enabled);
    m_logger->info("SequenceEngine",
        std::string("Debug mode ") + (enabled ? "enabled" : "disabled"));
}

bool SequenceEngine::isDebugMode() const
{
    return m_debugMode.load();
}

void SequenceEngine::addBreakpoint(const std::string& stateName)
{
    std::lock_guard<std::mutex> lock(m_debugMutex);
    m_breakpoints.insert(stateName);
    m_logger->debug("SequenceEngine", "Breakpoint added: " + stateName);
}

void SequenceEngine::removeBreakpoint(const std::string& stateName)
{
    std::lock_guard<std::mutex> lock(m_debugMutex);
    m_breakpoints.erase(stateName);
    m_logger->debug("SequenceEngine", "Breakpoint removed: " + stateName);
}

void SequenceEngine::clearBreakpoints()
{
    std::lock_guard<std::mutex> lock(m_debugMutex);
    m_breakpoints.clear();
    m_logger->debug("SequenceEngine", "All breakpoints cleared");
}

ErrorCode SequenceEngine::stepNext()
{
    if (!m_debugMode.load())
    {
        return ErrorCode::UnknownError;
    }

    EngineStatus currentStatus = m_status.load();
    if (currentStatus != EngineStatus::Paused)
    {
        return ErrorCode::EngineNotRunning;
    }

    m_stepRequested.store(true);
    m_stepCondition.notify_all();
    // pause를 풀어야 루프가 한 번 돌 수 있음
    m_pauseCondition.notify_all();

    m_logger->debug("SequenceEngine", "Step next requested");
    return ErrorCode::Success;
}

// ============================================================================
// 상태 접근자
// ============================================================================

EngineStatus SequenceEngine::getStatus() const
{
    return m_status.load();
}

std::string SequenceEngine::getCurrentStateName() const
{
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (m_currentState != nullptr)
    {
        return m_currentState->getName();
    }
    return "";
}

const std::string& SequenceEngine::getEngineName() const
{
    return m_engineName;
}

std::vector<std::string> SequenceEngine::getStateNames() const
{
    std::lock_guard<std::mutex> lock(m_engineMutex);

    std::vector<std::string> names;
    names.reserve(m_states.size());
    for (const auto& pair : m_states)
    {
        names.push_back(pair.first);
    }
    return names;
}

const IState* SequenceEngine::getState(const std::string& stateName) const
{
    std::lock_guard<std::mutex> lock(m_engineMutex);

    auto it = m_states.find(stateName);
    if (it != m_states.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::shared_ptr<Logger> SequenceEngine::getLogger() const
{
    return m_logger;
}

// ============================================================================
// 워커 스레드 실행 로직 (private)
// ============================================================================

void SequenceEngine::engineLoop()
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    m_logger->info("SequenceEngine",
        "Worker thread started. ThreadID=" + std::to_string(threadId));

    // 스레드 상태를 Running으로 통지
    notifyThreadStateChanged(ThreadState::Running, "");

    // 초기 상태 진입
    if (m_currentState != nullptr)
    {
        ErrorCode entryResult = m_currentState->onEntry();
        if (entryResult != ErrorCode::Success)
        {
            m_logger->error("SequenceEngine",
                "Initial state entry failed: " + m_currentState->getName() +
                " error=" + errorCodeToString(entryResult));
            notifyError(ErrorCode::StateEntryFailed,
                "Initial state entry failed: " + m_currentState->getName());
            setStatus(EngineStatus::Error);
            notifyThreadStateChanged(ThreadState::Terminated, "");
            return;
        }

        m_logger->info("SequenceEngine",
            "Entered initial state: " + m_currentState->getName());

        // 브레이크포인트 체크 (초기 상태)
        if (m_debugMode.load())
        {
            std::lock_guard<std::mutex> dbgLock(m_debugMutex);
            if (m_breakpoints.count(m_currentState->getName()) > 0)
            {
                m_logger->info("SequenceEngine",
                    "Breakpoint hit at initial state: " + m_currentState->getName());
                notifyDebugBreak(m_currentState->getName(), m_currentState->getParams());
                m_pauseRequested.store(true);
                setStatus(EngineStatus::Paused);
            }
        }
    }

    // === 메인 엔진 루프 ===
    while (!m_abortRequested.load())
    {
        // --- pause 처리 ---
        if (m_pauseRequested.load())
        {
            if (m_status.load() != EngineStatus::Paused)
            {
                setStatus(EngineStatus::Paused);
            }

            notifyThreadStateChanged(ThreadState::Waiting, "PauseCondition");

            // 디버그 스텝 모드: stepNext() 또는 resume() 대기
            std::unique_lock<std::mutex> pauseLock(m_pauseMutex);
            m_pauseCondition.wait(pauseLock, [this]()
            {
                return !m_pauseRequested.load() ||
                       m_abortRequested.load() ||
                       m_stepRequested.load();
            });

            // 스텝 실행인 경우: 한 사이클만 수행 후 다시 pause
            bool isStepping = m_stepRequested.load();
            if (isStepping)
            {
                m_stepRequested.store(false);
                // pause 상태 유지 (스텝 후 다시 대기)
            }
            else if (!m_abortRequested.load())
            {
                notifyThreadStateChanged(ThreadState::Running, "");
            }

            if (m_abortRequested.load())
            {
                break;
            }

            // resume인 경우 상태를 Running으로 변경 (스텝이 아닌 경우만)
            if (!isStepping && m_status.load() == EngineStatus::Paused)
            {
                setStatus(EngineStatus::Running);
                notifyThreadStateChanged(ThreadState::Running, "");
            }
        }

        // --- 전역 인터락 체크 ---
        {
            std::vector<InterlockCheckResult> globalResults;
            bool globalOk = m_interlockManager->checkAll(globalResults);

            // 인터락 결과를 Observer에 통지
            for (const auto& result : globalResults)
            {
                notifyInterlockChecked(result);
                if (!result.passed)
                {
                    notifyInterlockViolation(result);

                    // Critical 이상이면 pause
                    if (result.severity == InterlockSeverity::Critical)
                    {
                        m_logger->warn("SequenceEngine",
                            "Critical interlock violation: " + result.interlockName);
                        m_pauseRequested.store(true);
                        setStatus(EngineStatus::Paused);
                    }
                    // EMO는 EMO 콜백에서 처리 (abort 요청됨)
                }
            }

            if (m_abortRequested.load())
            {
                break;
            }
        }

        // --- 현재 상태 실행 ---
        if (m_currentState != nullptr)
        {
            ErrorCode execResult = m_currentState->onExecute();
            if (execResult != ErrorCode::Success)
            {
                m_logger->error("SequenceEngine",
                    "State execution failed: " + m_currentState->getName() +
                    " error=" + errorCodeToString(execResult));
                notifyError(ErrorCode::StateExecutionFailed,
                    "State execution failed: " + m_currentState->getName());
                setStatus(EngineStatus::Error);
                break;
            }
        }

        // --- 전이 평가 ---
        std::string nextStateName;
        bool transitionFound = evaluateTransitions(nextStateName);

        if (transitionFound)
        {
            // 전이 실행
            ErrorCode transResult = executeTransition(nextStateName);
            if (transResult != ErrorCode::Success)
            {
                m_logger->error("SequenceEngine",
                    "Transition execution failed to: " + nextStateName);
                setStatus(EngineStatus::Error);
                break;
            }

            // End 상태에 도달했으면 완료
            if (m_currentState != nullptr && m_currentState->getType() == StateType::End)
            {
                // End 상태의 onExecute()를 한 번 실행
                ErrorCode endResult = m_currentState->onExecute();
                if (endResult != ErrorCode::Success)
                {
                    m_logger->warn("SequenceEngine",
                        "End state execution returned: " + std::string(errorCodeToString(endResult)));
                }

                // End 상태의 onExit() 호출
                m_currentState->onExit();

                m_logger->info("SequenceEngine", "Sequence completed");
                setStatus(EngineStatus::Complete);
                break;
            }

            // 디버그 모드: 스텝 실행 후 다시 pause
            if (m_debugMode.load())
            {
                // 브레이크포인트 체크
                bool shouldPause = false;
                {
                    std::lock_guard<std::mutex> dbgLock(m_debugMutex);
                    if (m_currentState != nullptr &&
                        m_breakpoints.count(m_currentState->getName()) > 0)
                    {
                        shouldPause = true;
                    }
                }

                if (shouldPause && m_currentState != nullptr)
                {
                    m_logger->info("SequenceEngine",
                        "Breakpoint hit: " + m_currentState->getName());
                    notifyDebugBreak(m_currentState->getName(), m_currentState->getParams());
                    m_pauseRequested.store(true);
                    setStatus(EngineStatus::Paused);
                }
                // 스텝 모드에서는 매 전이 후 pause
                else if (m_status.load() == EngineStatus::Paused)
                {
                    // 이미 스텝 실행 1회 완료, 다시 pause 대기
                    m_pauseRequested.store(true);
                }
            }
        }

        // 엔진 루프 주기 대기 (CPU 부하 방지)
        std::this_thread::sleep_for(
            std::chrono::milliseconds(Constants::DEFAULT_ENGINE_LOOP_INTERVAL_MS));
    }

    // 워커 스레드 종료
    m_logger->info("SequenceEngine",
        "Worker thread ended. ThreadID=" + std::to_string(threadId));
    notifyThreadStateChanged(ThreadState::Terminated, "");
}

bool SequenceEngine::evaluateTransitions(std::string& selectedToState)
{
    if (m_currentState == nullptr)
    {
        return false;
    }

    const auto& transitions = m_currentState->getTransitions();
    if (transitions.empty())
    {
        return false;
    }

    // 우선순위 순서로 전이 정렬 (높은 우선순위 먼저)
    // 원본을 수정하지 않고 인덱스로 정렬
    std::vector<size_t> indices(transitions.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        indices[i] = i;
    }
    std::sort(indices.begin(), indices.end(),
        [&transitions](size_t a, size_t b)
        {
            return transitions[a]->getPriority() > transitions[b]->getPriority();
        });

    // 각 전이를 평가
    for (size_t idx : indices)
    {
        const auto& transition = transitions[idx];
        bool guardResult = false;
        std::vector<InterlockCheckResult> interlockResults;

        bool canTransit = transition->evaluate(guardResult, interlockResults);

        // Observer에 전이 평가 결과 통지 (성공/실패 모두)
        notifyTransitionEvaluated(
            transition->getFromState(),
            transition->getToState(),
            guardResult,
            interlockResults);

        if (canTransit)
        {
            selectedToState = transition->getToState();
            return true;
        }
    }

    return false;
}

ErrorCode SequenceEngine::executeTransition(const std::string& toStateName)
{
    // 도착 상태 찾기
    auto it = m_states.find(toStateName);
    if (it == m_states.end())
    {
        m_logger->error("SequenceEngine", "Transition target state not found: " + toStateName);
        return ErrorCode::StateNotFound;
    }

    std::string fromStateName;
    if (m_currentState != nullptr)
    {
        fromStateName = m_currentState->getName();

        // 현재 상태 퇴장
        ErrorCode exitResult = m_currentState->onExit();
        if (exitResult != ErrorCode::Success)
        {
            m_logger->error("SequenceEngine",
                "State exit failed: " + fromStateName +
                " error=" + errorCodeToString(exitResult));
            return ErrorCode::StateExitFailed;
        }
    }

    // 다음 상태로 전환
    IState* nextState = it->second.get();
    m_currentState = nextState;

    // 다음 상태 진입
    ErrorCode entryResult = m_currentState->onEntry();
    if (entryResult != ErrorCode::Success)
    {
        m_logger->error("SequenceEngine",
            "State entry failed: " + toStateName +
            " error=" + errorCodeToString(entryResult));
        return ErrorCode::StateEntryFailed;
    }

    m_logger->info("SequenceEngine",
        "State transition: " + fromStateName + " -> " + toStateName);

    // Observer에 상태 전이 통지
    // 전이가 성공했으므로 guard=true, interlock은 빈 목록 (이미 평가됨)
    notifyStateChanged(fromStateName, toStateName, true, {});

    return ErrorCode::Success;
}

void SequenceEngine::setStatus(EngineStatus newStatus)
{
    EngineStatus oldStatus = m_status.exchange(newStatus);
    if (oldStatus != newStatus)
    {
        m_logger->info("SequenceEngine",
            std::string("Engine status: ") + engineStatusToString(oldStatus) +
            " -> " + engineStatusToString(newStatus));

        // Observer에 엔진 상태 변경 통지
        uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
        std::lock_guard<std::mutex> lock(m_observerMutex);
        for (auto* observer : m_observers)
        {
            if (observer != nullptr)
            {
                observer->onEngineStatusChanged(threadId, newStatus);
            }
        }
    }
}

// ============================================================================
// Observer 통지 (private)
// ============================================================================

void SequenceEngine::notifyStateChanged(
    const std::string& fromState,
    const std::string& toState,
    bool guardResult,
    const std::vector<InterlockCheckResult>& interlockResults)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onStateChanged(threadId, fromState, toState, guardResult, interlockResults);
        }
    }
}

void SequenceEngine::notifyInterlockViolation(const InterlockCheckResult& result)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onInterlockViolation(
                threadId,
                result.interlockName,
                result.severity,
                result.description,
                result.currentValue,
                result.thresholdValue);
        }
    }
}

void SequenceEngine::notifyInterlockChecked(const InterlockCheckResult& result)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onInterlockChecked(
                threadId,
                result.interlockName,
                result.currentValue,
                result.passed);
        }
    }
}

void SequenceEngine::notifyError(ErrorCode errorCode, const std::string& description)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onError(threadId, errorCode, description);
        }
    }
}

void SequenceEngine::notifyThreadStateChanged(ThreadState newState, const std::string& lockInfo)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onThreadStateChanged(threadId, m_engineName, newState, lockInfo);
        }
    }
}

void SequenceEngine::notifyDebugBreak(
    const std::string& stateName,
    const std::map<std::string, ParamValue>& params)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onDebugBreak(threadId, stateName, params);
        }
    }
}

void SequenceEngine::notifyTransitionEvaluated(
    const std::string& fromState,
    const std::string& toState,
    bool guardResult,
    const std::vector<InterlockCheckResult>& interlockResults)
{
    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());
    std::lock_guard<std::mutex> lock(m_observerMutex);

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onTransitionEvaluated(
                threadId, fromState, toState, guardResult, interlockResults);
        }
    }
}

} // namespace YggdraSeq
