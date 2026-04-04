/**
 * @file HandlerSequence.cpp
 * @brief 핸들러 시퀀스 상태 구현 (Thread-B)
 *
 * 로봇 팔의 이동과 그리퍼 제어를 시뮬레이션합니다.
 */

#include "pch.h"
#include "HandlerSequence.h"
#include "SimulatedHardware.h"
#include <cmath>

namespace YggdraSeq
{

// ============================================================================
// HandlerStateBase 구현
// ============================================================================

HandlerStateBase::HandlerStateBase(const std::string& name, StateType type,
    SimulatedHardware* pHardware)
    : m_name(name)
    , m_type(type)
    , m_pHardware(pHardware)
    , m_stepComplete(false)
{
}

HandlerStateBase::~HandlerStateBase()
{
}

std::string HandlerStateBase::getName() const { return m_name; }
StateType HandlerStateBase::getType() const { return m_type; }

void HandlerStateBase::addTransition(std::unique_ptr<Transition> transition)
{
    m_transitions.push_back(std::move(transition));
}

const std::vector<std::unique_ptr<Transition>>& HandlerStateBase::getTransitions() const
{
    return m_transitions;
}

void HandlerStateBase::setParams(const std::map<std::string, ParamValue>& params)
{
    m_params = params;
}

const std::map<std::string, ParamValue>& HandlerStateBase::getParams() const
{
    return m_params;
}

// ============================================================================
// HandlerIdleState - 핸들러 대기
// ============================================================================

HandlerIdleState::HandlerIdleState(SimulatedHardware* pHardware)
    : HandlerStateBase("HandlerIdle", StateType::Wait, pHardware)
{
}

ErrorCode HandlerIdleState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode HandlerIdleState::onExecute()
{
    // 대기: 주기적으로 체크 (guard 조건에서 판단)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_stepComplete.store(true);  // guard에서 조건 판단
    return ErrorCode::Success;
}

ErrorCode HandlerIdleState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// PickState - 웨이퍼 픽업
// ============================================================================

PickState::PickState(SimulatedHardware* pHardware)
    : HandlerStateBase("Pick", StateType::Process, pHardware)
    , m_arrived(false)
{
}

ErrorCode PickState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    m_arrived = false;
    return ErrorCode::Success;
}

ErrorCode PickState::onExecute()
{
    double curX = m_pHardware->getArmX();
    double curY = m_pHardware->getArmY();

    bool xDone = std::abs(curX - PICK_X) < 1.0;
    bool yDone = std::abs(curY - PICK_Y) < 1.0;

    if (!m_arrived)
    {
        if (xDone && yDone)
        {
            // 도착 -> 정밀 위치 보정
            m_pHardware->setArmX(PICK_X);
            m_pHardware->setArmY(PICK_Y);
            m_arrived = true;
            // 그리퍼 파지
            m_pHardware->setGripState(true);
        }
        else
        {
            // 점진 이동
            if (!xDone)
            {
                double diff = PICK_X - curX;
                double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
                m_pHardware->setArmX(curX + step);
            }
            if (!yDone)
            {
                double diff = PICK_Y - curY;
                double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
                m_pHardware->setArmY(curY + step);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    else
    {
        // 그리퍼 파지 대기 (0.2초)
        auto elapsed = std::chrono::steady_clock::now() - m_entryTime;
        if (elapsed >= std::chrono::milliseconds(800))
        {
            m_stepComplete.store(true);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    return ErrorCode::Success;
}

ErrorCode PickState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// MoveState - 웨이퍼 이송
// ============================================================================

MoveState::MoveState(SimulatedHardware* pHardware)
    : HandlerStateBase("Move", StateType::Process, pHardware)
{
}

ErrorCode MoveState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode MoveState::onExecute()
{
    double curX = m_pHardware->getArmX();
    double curY = m_pHardware->getArmY();

    bool xDone = std::abs(curX - OUTPUT_X) < 1.0;
    bool yDone = std::abs(curY - OUTPUT_Y) < 1.0;

    if (xDone && yDone)
    {
        m_pHardware->setArmX(OUTPUT_X);
        m_pHardware->setArmY(OUTPUT_Y);
        m_stepComplete.store(true);
    }
    else
    {
        if (!xDone)
        {
            double diff = OUTPUT_X - curX;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setArmX(curX + step);
        }
        if (!yDone)
        {
            double diff = OUTPUT_Y - curY;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setArmY(curY + step);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode MoveState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// PlaceState - 웨이퍼 배치
// ============================================================================

PlaceState::PlaceState(SimulatedHardware* pHardware)
    : HandlerStateBase("Place", StateType::Process, pHardware)
{
}

ErrorCode PlaceState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode PlaceState::onExecute()
{
    auto elapsed = std::chrono::steady_clock::now() - m_entryTime;

    if (elapsed >= std::chrono::milliseconds(300))
    {
        // 그리퍼 해제
        m_pHardware->setGripState(false);
        m_stepComplete.store(true);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode PlaceState::onExit()
{
    return ErrorCode::Success;
}

// ============================================================================
// ReturnState - 원점 복귀
// ============================================================================

ReturnState::ReturnState(SimulatedHardware* pHardware)
    : HandlerStateBase("Return", StateType::Process, pHardware)
{
}

ErrorCode ReturnState::onEntry()
{
    m_entryTime = std::chrono::steady_clock::now();
    m_stepComplete.store(false);
    return ErrorCode::Success;
}

ErrorCode ReturnState::onExecute()
{
    double curX = m_pHardware->getArmX();
    double curY = m_pHardware->getArmY();

    bool xDone = std::abs(curX - HOME_X) < 1.0;
    bool yDone = std::abs(curY - HOME_Y) < 1.0;

    if (xDone && yDone)
    {
        m_pHardware->setArmX(HOME_X);
        m_pHardware->setArmY(HOME_Y);
        m_stepComplete.store(true);
    }
    else
    {
        if (!xDone)
        {
            double diff = HOME_X - curX;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setArmX(curX + step);
        }
        if (!yDone)
        {
            double diff = HOME_Y - curY;
            double step = (std::abs(diff) > MOVE_SPEED) ? (diff > 0 ? MOVE_SPEED : -MOVE_SPEED) : diff;
            m_pHardware->setArmY(curY + step);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return ErrorCode::Success;
}

ErrorCode ReturnState::onExit()
{
    return ErrorCode::Success;
}

} // namespace YggdraSeq
