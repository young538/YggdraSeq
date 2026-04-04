/**
 * @file IState.h
 * @brief 상태 머신의 상태 인터페이스 (순수 가상 클래스)
 *
 * 시퀀스의 한 단계를 나타내는 추상 인터페이스입니다.
 * 각 공정 스텝(Load, Align, Capture, Inspect 등)을 이 인터페이스를
 * 상속하여 구현합니다.
 *
 * 상태 생명주기:
 *   onEntry() -> onExecute() [반복] -> onExit()
 *   - onEntry():   상태 진입 시 1회 호출 (초기화)
 *   - onExecute(): 상태 활성 중 매 엔진 루프마다 호출 (작업 수행)
 *   - onExit():    상태 퇴장 시 1회 호출 (정리)
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

namespace YggdraSeq
{

// 전방 선언
class Transition;

/**
 * @class IState
 * @brief 시퀀스 상태의 순수 가상 인터페이스
 *
 * 모든 공정 스텝은 이 인터페이스를 상속하여 구현해야 합니다.
 * SequenceEngine에 registerState()로 등록한 후 사용합니다.
 *
 * 구현 시 주의사항:
 * - onExecute()는 매 루프마다 호출되므로 비블로킹으로 작성하세요
 * - onEntry()/onExit()에서 리소스 획득/해제를 RAII 패턴으로 수행하세요
 * - 멤버변수 접근 시 스레드 안전성을 고려하세요
 */
class AFX_EXT_CLASS IState
{
public:
    /**
     * @brief 가상 소멸자
     */
    virtual ~IState() = default;

    // ========================================================================
    // 상태 생명주기 메서드 (순수 가상)
    // ========================================================================

    /**
     * @brief 상태 진입 시 1회 호출
     *
     * 상태에 필요한 리소스를 초기화하고 장비를 준비합니다.
     * 이 메서드가 ErrorCode::Success가 아닌 값을 반환하면
     * 상태 진입이 실패하고 엔진이 Error 상태로 전환됩니다.
     *
     * @return ErrorCode 진입 결과. Success = 정상 진입
     */
    virtual ErrorCode onEntry() = 0;

    /**
     * @brief 상태 활성 중 매 엔진 루프마다 호출
     *
     * 실제 공정 작업을 수행합니다.
     * 작업이 완료되면 ErrorCode::Success를 반환하여 전이 평가를 트리거합니다.
     * 작업이 진행 중이면 ErrorCode::Success를 반환하되, 전이 조건(guard)이
     * false를 반환하여 현재 상태에 머무릅니다.
     *
     * @return ErrorCode 실행 결과. Success = 정상 실행
     * @note 이 메서드 내에서 장시간 블로킹하지 마세요.
     *       타이머나 상태 플래그를 사용하여 비블로킹으로 작성하세요.
     */
    virtual ErrorCode onExecute() = 0;

    /**
     * @brief 상태 퇴장 시 1회 호출
     *
     * 상태에서 사용한 리소스를 정리합니다.
     * 다음 상태로 전이하기 직전에 호출됩니다.
     *
     * @return ErrorCode 퇴장 결과. Success = 정상 퇴장
     */
    virtual ErrorCode onExit() = 0;

    // ========================================================================
    // 상태 정보 접근자 (순수 가상)
    // ========================================================================

    /**
     * @brief 상태의 고유 이름을 반환
     *
     * 이 이름은 SequenceEngine 내에서 상태를 식별하는 키로 사용됩니다.
     * 중복된 이름의 상태는 등록할 수 없습니다.
     *
     * @return 상태 고유 이름 (예: "Align", "Capture", "Inspect")
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 상태의 유형을 반환
     *
     * 모니터링 뷰에서 상태를 시각적으로 구분하는 데 사용됩니다.
     *
     * @return 상태 유형 (Init, Process, Wait, End)
     */
    virtual StateType getType() const = 0;

    // ========================================================================
    // 전이 관리
    // ========================================================================

    /**
     * @brief 이 상태에서 나가는 전이를 추가
     *
     * 하나의 상태에서 여러 전이를 등록할 수 있습니다.
     * 엔진이 전이를 평가할 때 우선순위(priority) 순서로 체크합니다.
     *
     * @param transition 추가할 전이 (소유권 이전, unique_ptr)
     * @note 전이의 fromState는 이 상태의 이름과 일치해야 합니다.
     */
    virtual void addTransition(std::unique_ptr<Transition> transition) = 0;

    /**
     * @brief 등록된 모든 전이 목록을 반환
     *
     * @return 전이 목록에 대한 const 참조
     */
    virtual const std::vector<std::unique_ptr<Transition>>& getTransitions() const = 0;

    // ========================================================================
    // 파라미터 관리 (레시피 연동용)
    // ========================================================================

    /**
     * @brief 레시피에서 주입된 파라미터를 설정
     *
     * RecipeStep의 params가 이 메서드를 통해 상태에 주입됩니다.
     * onEntry() 이전에 호출됩니다.
     *
     * @param params 파라미터 맵 (key: 파라미터 이름, value: 값)
     */
    virtual void setParams(const std::map<std::string, ParamValue>& params) = 0;

    /**
     * @brief 현재 설정된 파라미터를 반환
     *
     * 디버그 모드에서 변수 워치에 활용됩니다.
     *
     * @return 현재 파라미터 맵에 대한 const 참조
     */
    virtual const std::map<std::string, ParamValue>& getParams() const = 0;
};

} // namespace YggdraSeq
