/**
 * @file test_Transition.cpp
 * @brief Transition 클래스 단위 테스트
 */

#include "pch.h"
#include "Transition.h"
#include "Interlock.h"

using namespace YggdraSeq;

// ============================================================================
// 기본 생성 및 접근자
// ============================================================================

TEST(TransitionTest, BasicConstruction)
{
    Transition t("StateA", "StateB", 5);

    EXPECT_EQ(t.getFromState(), "StateA");
    EXPECT_EQ(t.getToState(), "StateB");
    EXPECT_EQ(t.getPriority(), 5);
}

TEST(TransitionTest, DefaultPriorityIsZero)
{
    Transition t("A", "B");
    EXPECT_EQ(t.getPriority(), 0);
}

// ============================================================================
// Guard 조건 테스트
// ============================================================================

// guard 미설정 시 evaluate는 true (항상 전이 가능)
TEST(TransitionTest, NoGuardAlwaysPasses)
{
    Transition t("A", "B");
    bool guardResult = false;
    std::vector<InterlockCheckResult> ilResults;

    bool canTransit = t.evaluate(guardResult, ilResults);

    EXPECT_TRUE(canTransit);
    EXPECT_TRUE(guardResult);
}

// guard가 true를 반환하면 전이 가능
TEST(TransitionTest, GuardTrueAllowsTransition)
{
    Transition t("A", "B");
    t.setGuard([]() { return true; });

    bool guardResult = false;
    std::vector<InterlockCheckResult> ilResults;

    EXPECT_TRUE(t.evaluate(guardResult, ilResults));
    EXPECT_TRUE(guardResult);
}

// guard가 false를 반환하면 전이 불가
TEST(TransitionTest, GuardFalseBlocksTransition)
{
    Transition t("A", "B");
    t.setGuard([]() { return false; });

    bool guardResult = true;
    std::vector<InterlockCheckResult> ilResults;

    EXPECT_FALSE(t.evaluate(guardResult, ilResults));
    EXPECT_FALSE(guardResult);
}

// 동적 guard: 외부 변수에 따라 결과 변경
TEST(TransitionTest, DynamicGuard)
{
    bool ready = false;
    Transition t("Idle", "Run");
    t.setGuard([&ready]() { return ready; });

    bool g; std::vector<InterlockCheckResult> il;
    EXPECT_FALSE(t.evaluate(g, il));

    ready = true;
    EXPECT_TRUE(t.evaluate(g, il));
}

// ============================================================================
// 인터락 바인딩 테스트
// ============================================================================

// 인터락 통과 시 전이 가능
TEST(TransitionTest, InterlockPassAllowsTransition)
{
    Interlock il("Safe", []() { return true; }, InterlockSeverity::Warning);

    Transition t("A", "B");
    t.addInterlock(&il);

    bool guardResult;
    std::vector<InterlockCheckResult> ilResults;
    bool canTransit = t.evaluate(guardResult, ilResults);

    EXPECT_TRUE(canTransit);
    EXPECT_EQ(ilResults.size(), 1u);
    EXPECT_TRUE(ilResults[0].passed);
}

// 인터락 실패 시 전이 불가
TEST(TransitionTest, InterlockFailBlocksTransition)
{
    Interlock il("Danger", []() { return false; }, InterlockSeverity::Critical);

    Transition t("A", "B");
    t.addInterlock(&il);

    bool guardResult;
    std::vector<InterlockCheckResult> ilResults;
    bool canTransit = t.evaluate(guardResult, ilResults);

    EXPECT_FALSE(canTransit);
    EXPECT_TRUE(guardResult);  // guard는 통과 (미설정)
    EXPECT_FALSE(ilResults[0].passed);  // 인터락이 실패
}

// guard 통과 + 인터락 실패 = 전이 불가
TEST(TransitionTest, GuardPassInterlockFail)
{
    Interlock il("Block", []() { return false; }, InterlockSeverity::Warning);

    Transition t("A", "B");
    t.setGuard([]() { return true; });
    t.addInterlock(&il);

    bool guardResult;
    std::vector<InterlockCheckResult> ilResults;
    EXPECT_FALSE(t.evaluate(guardResult, ilResults));
    EXPECT_TRUE(guardResult);
}

// 복수 인터락 - 하나만 실패해도 전이 불가
TEST(TransitionTest, MultipleInterlocksOneFailBlocks)
{
    Interlock ok("OK", []() { return true; }, InterlockSeverity::Warning);
    Interlock fail("FAIL", []() { return false; }, InterlockSeverity::Critical);

    Transition t("A", "B");
    t.addInterlock(&ok);
    t.addInterlock(&fail);

    bool guardResult;
    std::vector<InterlockCheckResult> ilResults;
    EXPECT_FALSE(t.evaluate(guardResult, ilResults));
    EXPECT_EQ(ilResults.size(), 2u);
}
