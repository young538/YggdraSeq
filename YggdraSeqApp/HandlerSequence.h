/**
 * @file HandlerSequence.h
 * @brief 핸들러 시퀀스 상태 구현 (Thread-B)
 *
 * 외관검사 장비의 웨이퍼 핸들링 시퀀스를 IState 인터페이스를 상속하여 구현합니다.
 *
 * 시퀀스 흐름:
 *   HandlerIdle -> Pick -> Move -> Place -> Return -> HandlerIdle (반복)
 *
 * 각 State에서 SimulatedHardware의 로봇 팔 위치와 그리퍼를 제어합니다.
 * Inspection 시퀀스와 인터락을 공유하여 검사 중 Handler 동작을 차단합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "../YggdraSeqCore/IState.h"
#include "../YggdraSeqCore/Transition.h"
#include <memory>
#include <atomic>
#include <chrono>

namespace YggdraSeq
{

// 전방 선언
class SimulatedHardware;

// ============================================================================
// 핸들러 상태 베이스 클래스
// ============================================================================

/**
 * @class HandlerStateBase
 * @brief 핸들러 시퀀스 상태의 공통 베이스 클래스
 */
class HandlerStateBase : public IState
{
public:
    HandlerStateBase(const std::string& name, StateType type,
        SimulatedHardware* pHardware);
    virtual ~HandlerStateBase();

    std::string getName() const override;
    StateType getType() const override;
    void addTransition(std::unique_ptr<Transition> transition) override;
    const std::vector<std::unique_ptr<Transition>>& getTransitions() const override;
    void setParams(const std::map<std::string, ParamValue>& params) override;
    const std::map<std::string, ParamValue>& getParams() const override;

protected:
    std::string m_name;
    StateType m_type;
    SimulatedHardware* m_pHardware;
    std::vector<std::unique_ptr<Transition>> m_transitions;
    std::map<std::string, ParamValue> m_params;
    std::chrono::steady_clock::time_point m_entryTime;
    std::atomic<bool> m_stepComplete;
};

// ============================================================================
// 구체 상태 클래스들
// ============================================================================

/**
 * @class HandlerIdleState
 * @brief 핸들러 대기 상태
 *
 * 로봇 팔이 안전 위치(X=300mm)에서 대기합니다.
 * 검사 완료 신호를 받으면 Pick으로 전이합니다.
 */
class HandlerIdleState : public HandlerStateBase
{
public:
    HandlerIdleState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;
};

/**
 * @class PickState
 * @brief 웨이퍼 픽업 상태
 *
 * 로봇 팔을 스테이지 위치로 이동하고 웨이퍼를 파지합니다.
 * 시뮬레이션: armX -> 125mm (스테이지 위치), gripState -> true
 * 소요 시간: 약 0.8초
 */
class PickState : public HandlerStateBase
{
public:
    PickState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    static constexpr double PICK_X = 125.0;     ///< 픽업 위치 X
    static constexpr double PICK_Y = 80.0;      ///< 픽업 위치 Y
    static constexpr double MOVE_SPEED = 15.0;   ///< 이동 속도 (mm/step)
    bool m_arrived;                              ///< 도착 여부
};

/**
 * @class MoveState
 * @brief 웨이퍼 이송 상태
 *
 * 파지한 웨이퍼를 배출 카세트 위치로 이동합니다.
 * 시뮬레이션: armX -> 400mm (배출 카세트 위치)
 * 소요 시간: 약 0.8초
 */
class MoveState : public HandlerStateBase
{
public:
    MoveState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    static constexpr double OUTPUT_X = 400.0;   ///< 배출 위치 X
    static constexpr double OUTPUT_Y = 0.0;     ///< 배출 위치 Y
    static constexpr double MOVE_SPEED = 15.0;
};

/**
 * @class PlaceState
 * @brief 웨이퍼 배치 상태
 *
 * 웨이퍼를 배출 카세트에 놓습니다.
 * 시뮬레이션: gripState -> false
 * 소요 시간: 약 0.3초
 */
class PlaceState : public HandlerStateBase
{
public:
    PlaceState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;
};

/**
 * @class ReturnState
 * @brief 로봇 팔 원점 복귀 상태
 *
 * 로봇 팔을 안전 위치(X=300mm)로 복귀시킵니다.
 * 시뮬레이션: armX -> 300mm
 * 소요 시간: 약 0.5초
 */
class ReturnState : public HandlerStateBase
{
public:
    ReturnState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    static constexpr double HOME_X = 300.0;     ///< 홈 위치 X
    static constexpr double HOME_Y = 0.0;       ///< 홈 위치 Y
    static constexpr double MOVE_SPEED = 15.0;
};

} // namespace YggdraSeq
