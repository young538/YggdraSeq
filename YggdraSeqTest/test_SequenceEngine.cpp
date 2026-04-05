/**
 * @file test_SequenceEngine.cpp
 * @brief SequenceEngine 단위 테스트
 *
 * 테스트용 Mock 상태 클래스를 정의하고,
 * 엔진의 생명주기(run/pause/resume/abort), 상태 등록,
 * 전이, Observer 통지를 테스트합니다.
 */

#include "pch.h"
#include "SequenceEngine.h"
#include "IState.h"
#include "Transition.h"
#include "ISequenceObserver.h"
#include "InterlockManager.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace YggdraSeq;

// ============================================================================
// 테스트용 Mock 상태 클래스
// ============================================================================

class MockState : public IState
{
public:
    MockState(const std::string& name, StateType type = StateType::Process)
        : m_name(name), m_type(type), m_entryCount(0), m_executeCount(0), m_exitCount(0) {}

    ErrorCode onEntry() override { m_entryCount++; return ErrorCode::Success; }
    ErrorCode onExecute() override { m_executeCount++; return ErrorCode::Success; }
    ErrorCode onExit() override { m_exitCount++; return ErrorCode::Success; }

    std::string getName() const override { return m_name; }
    StateType getType() const override { return m_type; }

    void addTransition(std::unique_ptr<Transition> t) override { m_transitions.push_back(std::move(t)); }
    const std::vector<std::unique_ptr<Transition>>& getTransitions() const override { return m_transitions; }

    void setParams(const std::map<std::string, ParamValue>& p) override { m_params = p; }
    const std::map<std::string, ParamValue>& getParams() const override { return m_params; }

    int m_entryCount;
    int m_executeCount;
    int m_exitCount;

private:
    std::string m_name;
    StateType m_type;
    std::vector<std::unique_ptr<Transition>> m_transitions;
    std::map<std::string, ParamValue> m_params;
};

// 실행 횟수를 세다가 일정 횟수 후 전이 가능해지는 상태
class CountingState : public MockState
{
public:
    CountingState(const std::string& name, int maxCount, StateType type = StateType::Process)
        : MockState(name, type), m_maxCount(maxCount) {}

    ErrorCode onExecute() override
    {
        MockState::onExecute();
        return ErrorCode::Success;
    }

    bool isReady() const { return m_executeCount >= m_maxCount; }

private:
    int m_maxCount;
};

// ============================================================================
// 테스트용 Mock Observer
// ============================================================================

class MockObserver : public ISequenceObserver
{
public:
    MockObserver() : stateChangedCount(0), engineStatusCount(0), errorCount(0) {}

    void onStateChanged(uint32_t, const std::string& from, const std::string& to,
        bool, const std::vector<InterlockCheckResult>&) override
    {
        stateChangedCount++;
        lastFromState = from;
        lastToState = to;
    }

    void onEngineStatusChanged(uint32_t, EngineStatus s) override
    {
        engineStatusCount++;
        lastStatus = s;
    }

    void onInterlockViolation(uint32_t, const std::string&, InterlockSeverity,
        const std::string&, const std::string&, const std::string&) override {}
    void onInterlockChecked(uint32_t, const std::string&, const std::string&, bool) override {}
    void onLogMessage(uint32_t, LogLevel, const std::string&, const std::string&) override {}
    void onError(uint32_t, ErrorCode, const std::string&) override { errorCount++; }
    void onThreadStateChanged(uint32_t, const std::string&, ThreadState, const std::string&) override {}
    void onDebugBreak(uint32_t, const std::string&, const std::map<std::string, ParamValue>&) override {}
    void onTransitionEvaluated(uint32_t, const std::string&, const std::string&,
        bool, const std::vector<InterlockCheckResult>&) override {}

    std::atomic<int> stateChangedCount;
    std::atomic<int> engineStatusCount;
    std::atomic<int> errorCount;
    std::string lastFromState;
    std::string lastToState;
    EngineStatus lastStatus = EngineStatus::Idle;
};

// ============================================================================
// 헬퍼: 엔진이 특정 상태가 될 때까지 대기
// ============================================================================

static bool waitForStatus(SequenceEngine& engine, EngineStatus status, int timeoutMs = 3000)
{
    auto start = std::chrono::steady_clock::now();
    while (engine.getStatus() != status)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeoutMs)
            return false;
    }
    return true;
}

// ============================================================================
// 상태 등록 테스트
// ============================================================================

TEST(SequenceEngineTest, InitialStatusIsIdle)
{
    SequenceEngine engine("Test");
    EXPECT_EQ(engine.getStatus(), EngineStatus::Idle);
    EXPECT_EQ(engine.getEngineName(), "Test");
}

TEST(SequenceEngineTest, RegisterState)
{
    SequenceEngine engine("Test");

    auto state = std::make_unique<MockState>("StateA");
    EXPECT_EQ(engine.registerState(std::move(state)), ErrorCode::Success);

    auto names = engine.getStateNames();
    EXPECT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "StateA");
}

TEST(SequenceEngineTest, RegisterDuplicateStateFails)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("A"));
    auto result = engine.registerState(std::make_unique<MockState>("A"));
    EXPECT_EQ(result, ErrorCode::StateAlreadyRegistered);
}

TEST(SequenceEngineTest, SetInitialState)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("Idle"));

    EXPECT_EQ(engine.setInitialState("Idle"), ErrorCode::Success);
    EXPECT_EQ(engine.setInitialState("NonExist"), ErrorCode::StateNotFound);
}

// ============================================================================
// 실행 제어 테스트
// ============================================================================

// 상태 없이 run 하면 에러
TEST(SequenceEngineTest, RunWithoutStatesFails)
{
    SequenceEngine engine("Test");
    EXPECT_NE(engine.run(), ErrorCode::Success);
}

// 초기 상태 없이 run 하면 에러
TEST(SequenceEngineTest, RunWithoutInitialStateFails)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("A"));
    EXPECT_NE(engine.run(), ErrorCode::Success);
}

// 정상 실행 후 상태가 Running으로 변경
TEST(SequenceEngineTest, RunChangesStatusToRunning)
{
    SequenceEngine engine("Test");
    auto state = std::make_unique<MockState>("Idle");
    engine.registerState(std::move(state));
    engine.setInitialState("Idle");

    EXPECT_EQ(engine.run(), ErrorCode::Success);
    EXPECT_TRUE(waitForStatus(engine, EngineStatus::Running));

    engine.abort();
}

// 이미 실행 중일 때 다시 run 하면 에러
TEST(SequenceEngineTest, DoubleRunFails)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("Idle"));
    engine.setInitialState("Idle");
    engine.run();
    waitForStatus(engine, EngineStatus::Running);

    EXPECT_EQ(engine.run(), ErrorCode::EngineAlreadyRunning);

    engine.abort();
}

// pause / resume
TEST(SequenceEngineTest, PauseAndResume)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("Idle"));
    engine.setInitialState("Idle");
    engine.run();
    waitForStatus(engine, EngineStatus::Running);

    EXPECT_EQ(engine.pause(), ErrorCode::Success);
    EXPECT_TRUE(waitForStatus(engine, EngineStatus::Paused));

    EXPECT_EQ(engine.resume(), ErrorCode::Success);
    EXPECT_TRUE(waitForStatus(engine, EngineStatus::Running));

    engine.abort();
}

// abort 후 재시작
TEST(SequenceEngineTest, AbortAndRestart)
{
    SequenceEngine engine("Test");
    engine.registerState(std::make_unique<MockState>("Idle"));
    engine.setInitialState("Idle");

    engine.run();
    waitForStatus(engine, EngineStatus::Running);
    engine.abort();
    waitForStatus(engine, EngineStatus::Aborted);

    // 재시작 가능해야 함
    EXPECT_EQ(engine.run(), ErrorCode::Success);
    EXPECT_TRUE(waitForStatus(engine, EngineStatus::Running));

    engine.abort();
}

// ============================================================================
// 상태 전이 테스트
// ============================================================================

TEST(SequenceEngineTest, SimpleTransition)
{
    SequenceEngine engine("Test");

    auto stateA = std::make_unique<CountingState>("A", 3);
    CountingState* pA = stateA.get();
    auto tAtoB = std::make_unique<Transition>("A", "B");
    tAtoB->setGuard([pA]() { return pA->isReady(); });
    stateA->addTransition(std::move(tAtoB));

    auto stateB = std::make_unique<MockState>("B", StateType::End);

    engine.registerState(std::move(stateA));
    engine.registerState(std::move(stateB));
    engine.setInitialState("A");

    engine.run();

    // End 상태(B)에 도달하면 Complete
    EXPECT_TRUE(waitForStatus(engine, EngineStatus::Complete, 5000));
    EXPECT_TRUE(pA->m_executeCount >= 3);
}

// 순환 전이 (A → B → A → B → ...)
TEST(SequenceEngineTest, CyclicTransition)
{
    SequenceEngine engine("Test");
    std::atomic<int> cycleCount{0};

    auto stateA = std::make_unique<CountingState>("A", 2);
    CountingState* pA = stateA.get();
    auto tAtoB = std::make_unique<Transition>("A", "B");
    tAtoB->setGuard([pA]() { return pA->isReady(); });
    stateA->addTransition(std::move(tAtoB));

    auto stateB = std::make_unique<MockState>("B");
    auto tBtoA = std::make_unique<Transition>("B", "A");
    tBtoA->setGuard([&cycleCount]() {
        cycleCount++;
        return true;  // 즉시 A로 돌아감
    });
    stateB->addTransition(std::move(tBtoA));

    engine.registerState(std::move(stateA));
    engine.registerState(std::move(stateB));
    engine.setInitialState("A");

    engine.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_GT(cycleCount.load(), 0);  // 최소 1번은 순환
    engine.abort();
}

// ============================================================================
// Observer 통지 테스트
// ============================================================================

TEST(SequenceEngineTest, ObserverNotified)
{
    SequenceEngine engine("Test");
    MockObserver observer;
    engine.addObserver(&observer);

    auto stateA = std::make_unique<MockState>("A");
    auto tAtoB = std::make_unique<Transition>("A", "B");
    tAtoB->setGuard([]() { return true; });  // 즉시 전이
    stateA->addTransition(std::move(tAtoB));

    auto stateB = std::make_unique<MockState>("B", StateType::End);

    engine.registerState(std::move(stateA));
    engine.registerState(std::move(stateB));
    engine.setInitialState("A");

    engine.run();
    waitForStatus(engine, EngineStatus::Complete, 5000);

    EXPECT_GT(observer.stateChangedCount.load(), 0);
    EXPECT_GT(observer.engineStatusCount.load(), 0);

    engine.removeObserver(&observer);
}

// ============================================================================
// 디버그 모드 테스트
// ============================================================================

TEST(SequenceEngineTest, DebugModeDefault)
{
    SequenceEngine engine("Test");
    EXPECT_FALSE(engine.isDebugMode());

    engine.setDebugMode(true);
    EXPECT_TRUE(engine.isDebugMode());
}

TEST(SequenceEngineTest, BreakpointManagement)
{
    SequenceEngine engine("Test");

    engine.addBreakpoint("StateA");
    engine.addBreakpoint("StateB");
    engine.clearBreakpoints();
    // clearBreakpoints 후 에러 없이 동작해야 함
    EXPECT_TRUE(true);
}

// ============================================================================
// InterlockManager 접근 테스트
// ============================================================================

TEST(SequenceEngineTest, InterlockManagerAccess)
{
    SequenceEngine engine("Test");
    auto& mgr = engine.getInterlockManager();

    auto il = std::make_unique<Interlock>("TestIL", []() { return true; }, InterlockSeverity::Warning);
    EXPECT_EQ(mgr.addInterlock(std::move(il)), ErrorCode::Success);
    EXPECT_EQ(mgr.getInterlockCount(), 1u);
}
