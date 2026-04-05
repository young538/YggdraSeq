/**
 * @file test_Interlock.cpp
 * @brief Interlock / InterlockManager 단위 테스트
 */

#include "pch.h"
#include "Interlock.h"
#include "InterlockManager.h"

using namespace YggdraSeq;

// ============================================================================
// Interlock 기본 테스트
// ============================================================================

// 안전 조건 true일 때 check가 통과하는지
TEST(InterlockTest, CheckPassesWhenConditionTrue)
{
    Interlock il("TestIL", []() { return true; }, InterlockSeverity::Warning);
    auto result = il.check();

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.interlockName, "TestIL");
    EXPECT_EQ(result.severity, InterlockSeverity::Warning);
}

// 안전 조건 false일 때 check가 실패하는지
TEST(InterlockTest, CheckFailsWhenConditionFalse)
{
    Interlock il("DoorOpen", []() { return false; }, InterlockSeverity::Critical,
        "Door must be closed");
    auto result = il.check();

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.interlockName, "DoorOpen");
    EXPECT_EQ(result.severity, InterlockSeverity::Critical);
}

// 심각도별 생성 확인
TEST(InterlockTest, SeverityLevels)
{
    Interlock w("W", []() { return true; }, InterlockSeverity::Warning);
    Interlock c("C", []() { return true; }, InterlockSeverity::Critical);
    Interlock e("E", []() { return true; }, InterlockSeverity::EMO);

    EXPECT_EQ(w.getSeverity(), InterlockSeverity::Warning);
    EXPECT_EQ(c.getSeverity(), InterlockSeverity::Critical);
    EXPECT_EQ(e.getSeverity(), InterlockSeverity::EMO);
}

// 접근자 테스트 (이름, 설명, 현재값, 임계값)
TEST(InterlockTest, Accessors)
{
    Interlock il("Temp", []() { return true; }, InterlockSeverity::Warning, "Temperature check");
    il.setCurrentValue("85.5C");
    il.setThresholdValue("100.0C");

    EXPECT_EQ(il.getName(), "Temp");
    EXPECT_EQ(il.getDescription(), "Temperature check");
    EXPECT_EQ(il.getCurrentValue(), "85.5C");
    EXPECT_EQ(il.getThresholdValue(), "100.0C");
}

// 동적 조건 변경에 따른 check 결과 변화
TEST(InterlockTest, DynamicConditionChange)
{
    bool safe = true;
    Interlock il("Dynamic", [&safe]() { return safe; }, InterlockSeverity::Warning);

    EXPECT_TRUE(il.check().passed);

    safe = false;
    EXPECT_FALSE(il.check().passed);

    safe = true;
    EXPECT_TRUE(il.check().passed);
}

// ============================================================================
// InterlockManager 테스트
// ============================================================================

// 인터락 추가 및 카운트
TEST(InterlockManagerTest, AddAndCount)
{
    InterlockManager mgr;
    EXPECT_EQ(mgr.getInterlockCount(), 0u);

    auto il = std::make_unique<Interlock>("IL1", []() { return true; }, InterlockSeverity::Warning);
    EXPECT_EQ(mgr.addInterlock(std::move(il)), ErrorCode::Success);
    EXPECT_EQ(mgr.getInterlockCount(), 1u);
}

// 중복 이름 등록 실패
TEST(InterlockManagerTest, DuplicateNameFails)
{
    InterlockManager mgr;
    mgr.addInterlock(std::make_unique<Interlock>("IL1", []() { return true; }, InterlockSeverity::Warning));
    auto result = mgr.addInterlock(
        std::make_unique<Interlock>("IL1", []() { return true; }, InterlockSeverity::Critical));

    EXPECT_EQ(result, ErrorCode::InterlockAlreadyRegistered);
    EXPECT_EQ(mgr.getInterlockCount(), 1u);
}

// 전체 체크 - 모두 통과
TEST(InterlockManagerTest, CheckAllPass)
{
    InterlockManager mgr;
    mgr.addInterlock(std::make_unique<Interlock>("A", []() { return true; }, InterlockSeverity::Warning));
    mgr.addInterlock(std::make_unique<Interlock>("B", []() { return true; }, InterlockSeverity::Critical));

    std::vector<InterlockCheckResult> results;
    bool allOk = mgr.checkAll(results);

    EXPECT_TRUE(allOk);
    EXPECT_EQ(results.size(), 2u);
    for (const auto& r : results)
        EXPECT_TRUE(r.passed);
}

// 전체 체크 - 하나 실패
TEST(InterlockManagerTest, CheckAllWithFailure)
{
    InterlockManager mgr;
    mgr.addInterlock(std::make_unique<Interlock>("OK", []() { return true; }, InterlockSeverity::Warning));
    mgr.addInterlock(std::make_unique<Interlock>("FAIL", []() { return false; }, InterlockSeverity::Critical));

    std::vector<InterlockCheckResult> results;
    bool allOk = mgr.checkAll(results);

    EXPECT_FALSE(allOk);
    // 실패한 항목 확인
    bool foundFail = false;
    for (const auto& r : results)
    {
        if (r.interlockName == "FAIL")
        {
            EXPECT_FALSE(r.passed);
            foundFail = true;
        }
    }
    EXPECT_TRUE(foundFail);
}

// 인터락 제거
TEST(InterlockManagerTest, RemoveInterlock)
{
    InterlockManager mgr;
    mgr.addInterlock(std::make_unique<Interlock>("A", []() { return true; }, InterlockSeverity::Warning));
    mgr.addInterlock(std::make_unique<Interlock>("B", []() { return true; }, InterlockSeverity::Warning));
    EXPECT_EQ(mgr.getInterlockCount(), 2u);

    mgr.removeInterlock("A");
    EXPECT_EQ(mgr.getInterlockCount(), 1u);
}
