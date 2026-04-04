/**
 * @file InspectionSequence.h
 * @brief 검사 시퀀스 상태 구현 (Thread-A)
 *
 * 외관검사 장비의 검사 공정 시퀀스를 IState 인터페이스를 상속하여 구현합니다.
 *
 * 시퀀스 흐름:
 *   InspIdle -> Load -> Align -> Capture -> Inspect -> Judge -> InspIdle (반복)
 *
 * 각 State에서 SimulatedHardware 값을 점진적으로 변경하여
 * 실제 장비 동작을 시뮬레이션합니다.
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
// 기본 검사 상태 (공통 기능 포함)
// ============================================================================

/**
 * @class InspectionStateBase
 * @brief 검사 시퀀스 상태의 공통 베이스 클래스
 *
 * IState의 공통 기능(이름, 타입, 전이/파라미터 관리)을 구현합니다.
 * 각 구체 상태 클래스는 onEntry/onExecute/onExit만 구현하면 됩니다.
 */
class InspectionStateBase : public IState
{
public:
    /**
     * @brief 생성자
     * @param name 상태 이름
     * @param type 상태 유형
     * @param pHardware 시뮬레이션 하드웨어 참조 (소유권 없음)
     */
    InspectionStateBase(const std::string& name, StateType type,
        SimulatedHardware* pHardware);
    virtual ~InspectionStateBase();

    std::string getName() const override;
    StateType getType() const override;
    void addTransition(std::unique_ptr<Transition> transition) override;
    const std::vector<std::unique_ptr<Transition>>& getTransitions() const override;
    void setParams(const std::map<std::string, ParamValue>& params) override;
    const std::map<std::string, ParamValue>& getParams() const override;

protected:
    std::string m_name;                 ///< 상태 이름
    StateType m_type;                   ///< 상태 유형
    SimulatedHardware* m_pHardware;     ///< 시뮬레이션 하드웨어 (관찰용)
    std::vector<std::unique_ptr<Transition>> m_transitions;  ///< 전이 목록
    std::map<std::string, ParamValue> m_params;              ///< 파라미터
    std::chrono::steady_clock::time_point m_entryTime;       ///< 진입 시간
    std::atomic<bool> m_stepComplete;   ///< 현재 스텝 완료 플래그
};

// ============================================================================
// 구체 상태 클래스들
// ============================================================================

/**
 * @class InspIdleState
 * @brief 검사 대기 상태 (Idle)
 *
 * 검사 시퀀스의 시작/끝 상태입니다.
 * 웨이퍼가 남아있으면 자동으로 Load로 전이합니다.
 */
class InspIdleState : public InspectionStateBase
{
public:
    InspIdleState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;
};

/**
 * @class LoadState
 * @brief 웨이퍼 로드 상태
 *
 * 카세트에서 웨이퍼를 스테이지로 로드합니다.
 * 시뮬레이션: waferCount 감소, waferOnStage = true
 * 소요 시간: 약 0.5초
 */
class LoadState : public InspectionStateBase
{
public:
    LoadState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;
};

/**
 * @class AlignState
 * @brief 웨이퍼 정렬 상태
 *
 * 스테이지를 목표 위치(125, 80, 0)로 이동합니다.
 * 시뮬레이션: stageX/Y/Theta를 50ms마다 목표까지 점진 이동
 * 소요 시간: 약 1초
 */
class AlignState : public InspectionStateBase
{
public:
    AlignState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    static constexpr double TARGET_X = 125.0;       ///< 목표 X 위치
    static constexpr double TARGET_Y = 80.0;        ///< 목표 Y 위치
    static constexpr double TARGET_THETA = 0.0;     ///< 목표 회전 각도
    static constexpr double MOVE_SPEED = 10.0;      ///< 이동 속도 (mm/step)
};

/**
 * @class CaptureState
 * @brief 이미지 촬영 상태
 *
 * 카메라 초점을 맞추고 조명을 켜서 이미지를 촬영합니다.
 * 시뮬레이션: cameraFocus -> 12.5mm, lightIntensity -> 85%
 * 소요 시간: 약 0.5초
 */
class CaptureState : public InspectionStateBase
{
public:
    CaptureState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    static constexpr double TARGET_FOCUS = 12.5;    ///< 목표 초점 (mm)
    static constexpr double TARGET_LIGHT = 85.0;    ///< 목표 조명 밝기 (%)
};

/**
 * @class InspectState
 * @brief 검사 판정 상태
 *
 * 촬영된 이미지를 분석하여 Good/NG를 판정합니다.
 * 시뮬레이션: 랜덤 판정 (90% Good, 10% NG)
 * 소요 시간: 약 1초
 */
class InspectState : public InspectionStateBase
{
public:
    InspectState(SimulatedHardware* pHardware);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

    /** @brief 마지막 검사 결과 (true=Good, false=NG) */
    bool getLastResult() const { return m_lastResultGood; }

private:
    bool m_lastResultGood;  ///< 마지막 검사 결과
};

/**
 * @class JudgeState
 * @brief 판정 결과 처리 상태
 *
 * 검사 결과에 따라 Good/NG 카운터를 증가시키고
 * 조명을 끄고 카메라를 초기화합니다.
 * 소요 시간: 약 0.3초
 */
class JudgeState : public InspectionStateBase
{
public:
    JudgeState(SimulatedHardware* pHardware, InspectState* pInspectState);
    ErrorCode onEntry() override;
    ErrorCode onExecute() override;
    ErrorCode onExit() override;

private:
    InspectState* m_pInspectState;  ///< InspectState 참조 (판정 결과 조회용)
};

} // namespace YggdraSeq
