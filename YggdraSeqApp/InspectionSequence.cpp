/**
 * @file InspectionSequence.cpp
 * @brief 검사 시퀀스 상태 구현 (Thread-A)
 *
 * 각 상태에서 SimulatedHardware 값을 점진적으로 변경하여
 * 실제 장비 동작을 시뮬레이션합니다.
 */

#include "pch.h"
#include "InspectionSequence.h"
#include "SimulatedHardware.h"
#include <cmath>
#include <random>

namespace YggdraSeq
{

// ============================================================================
// InspectionStateBase 구현
// ============================================================================

InspectionStateBase::InspectionStateBase(const std::string& name, StateType type,
    SimulatedHardware* pHardware)
    : m_name(name)
    , m_type(type)
    , m_pHardware(pHardware)
    , m_stepComplete(false)
{
}

InspectionStateBase::~InspectionStateBase()
{
}

std::string InspectionStateBase::getName() const { return m_name; }
StateType InspectionStateBase::getType() const { return m_type; }

void InspectionStateBase::addTransition(std::unique_ptr<Transition> transition)
{
    m_transitions.push_back(std::move(transition));
}

const std::vector<std::unique_ptr<Transition>>& InspectionStateBase::getTransitions() const
{
    return m_transitions;
}

void InspectionStateBase::setParams(const std::map<std::string, ParamValue>& params)
{
    m_params = params;
}

const std::map<std::string, ParamValue>& InspectionStateBase::getParams() const
{
    return m_params;
}

// ============================================================================
// InspIdleState - 검사 대기 상태
// ============================================================================

InspIdleState::InspIdleState(SimulatedHardware* pHardware)
    : InspectionStateBase("InspIdle", StateType::Wait, pHardware)
{
}

ErrorCode InspIdleState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode InspIdleState::onExecute()
{
    // 웨이퍼가 남아있으면 자동으로 다음 사이클 시작
    if (m_pHardware->getWaferCount() > 0)
    {
        m_stepComplete.store(true);
    }
    else
    {
        // 웨이퍼 없으면 대기 (100ms sleep으로 CPU 절약)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return ErrorCode::Success;
}

ErrorCode InspIdleState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// LoadState - 웨이퍼 로드
// ============================================================================

LoadState::LoadState(SimulatedHardware* pHardware)
    : InspectionStateBase("Load", StateType::Process, pHardware)
{
}

ErrorCode LoadState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);

    // 웨이퍼 카운트 감소
    int count = m_pHardware->getWaferCount();
    if (count > 0)
    {
        m_pHardware->setWaferCount(count - 1);
    }

    return ErrorCode::Success;
}

ErrorCode LoadState::onExecute()
{
    // 로드 시뮬레이션: 0.5초 대기
    auto elapsed = std::chrono::steady_clock::now() - m_entryTime;
    if (elapsed >= std::chrono::milliseconds(500))
    {
        m_stepComplete.store(true);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return ErrorCode::Success;
}

ErrorCode LoadState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// AlignState - 웨이퍼 정렬
// ============================================================================

AlignState::AlignState(SimulatedHardware* pHardware)
    : InspectionStateBase("Align", StateType::Process, pHardware)
{
}

ErrorCode AlignState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode AlignState::onExecute()
{
    // 스테이지를 목표 위치로 점진 이동
    double curX = m_pHardware->getStageX();
    double curY = m_pHardware->getStageY();
    double curTheta = m_pHardware->getStageTheta();

    bool xDone = std::abs(curX - TARGET_X) < 0.5;
    bool yDone = std::abs(curY - TARGET_Y) < 0.5;
    bool tDone = std::abs(curTheta - TARGET_THETA) < 0.1;

    if (xDone && yDone && tDone)
    {
        // 정밀 보정
        m_pHardware->setStageX(TARGET_X);
        m_pHardware->setStageY(TARGET_Y);
        m_pHardware->setStageTheta(TARGET_THETA);
        m_stepComplete.store(true);
    }
    else
    {
        // 점진 이동
        if (!xDone)
        {
            double diff = TARGET_X - curX;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setStageX(curX + step);
        }
        if (!yDone)
        {
            double diff = TARGET_Y - curY;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setStageY(curY + step);
        }
        if (!tDone)
        {
            double diff = TARGET_THETA - curTheta;
            double step = (std::abs(diff) > 1.0) ? (diff > 0 ? 1.0 : -1.0) : diff;
            m_pHardware->setStageTheta(curTheta + step);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode AlignState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// CaptureState - 이미지 촬영
// ============================================================================

CaptureState::CaptureState(SimulatedHardware* pHardware)
    : InspectionStateBase("Capture", StateType::Process, pHardware)
{
}

ErrorCode CaptureState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode CaptureState::onExecute()
{
    auto elapsed = std::chrono::steady_clock::now() - m_entryTime;
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (elapsedMs < 200)
    {
        // 초점 조정 (0 -> 12.5mm)
        double progress = static_cast<double>(elapsedMs) / 200.0;
        m_pHardware->setCameraFocus(TARGET_FOCUS * progress);
        m_pHardware->setLightIntensity(TARGET_LIGHT * progress);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    else if (elapsedMs < 500)
    {
        // 촬영 시뮬레이션 (초점/조명 안정)
        m_pHardware->setCameraFocus(TARGET_FOCUS);
        m_pHardware->setLightIntensity(TARGET_LIGHT);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    else
    {
        m_stepComplete.store(true);
    }

    return ErrorCode::Success;
}

ErrorCode CaptureState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// InspectState - 검사 판정
// ============================================================================

InspectState::InspectState(SimulatedHardware* pHardware)
    : InspectionStateBase("Inspect", StateType::Process, pHardware)
    , m_lastResultGood(true)
{
}

ErrorCode InspectState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode InspectState::onExecute()
{
    auto elapsed = std::chrono::steady_clock::now() - m_entryTime;

    if (elapsed >= std::chrono::milliseconds(1000))
    {
        // 검사 판정 (90% Good, 10% NG)
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<int> dist(1, 100);
        m_lastResultGood = (dist(rng) <= 90);

        m_stepComplete.store(true);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode InspectState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// JudgeState - 판정 결과 처리
// ============================================================================

JudgeState::JudgeState(SimulatedHardware* pHardware, InspectState* pInspectState)
    : InspectionStateBase("Judge", StateType::Process, pHardware)
    , m_pInspectState(pInspectState)
{
}

ErrorCode JudgeState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);

    // 검사 결과에 따라 카운터 증가
    if (m_pInspectState != nullptr && m_pInspectState->getLastResult())
    {
        m_pHardware->incrementGoodCount();
    }
    else
    {
        m_pHardware->incrementNgCount();
    }

    return ErrorCode::Success;
}

ErrorCode JudgeState::onExecute()
{
    auto elapsed = std::chrono::steady_clock::now() - m_entryTime;

    if (elapsed >= std::chrono::milliseconds(300))
    {
        m_stepComplete.store(true);
    }
    else
    {
        // 조명 끄기 + 카메라 초기화
        double progress = 1.0 - (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 300.0);
        m_pHardware->setLightIntensity(85.0 * progress);
        m_pHardware->setCameraFocus(12.5 * progress);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode JudgeState::onExit()
{
    // 완전히 초기화
    m_pHardware->setLightIntensity(0.0);
    m_pHardware->setCameraFocus(0.0);
    return ErrorCode::Success;
}

} // namespace YggdraSeq
