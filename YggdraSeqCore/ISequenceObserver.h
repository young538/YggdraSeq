/**
 * @file ISequenceObserver.h
 * @brief 시퀀스 엔진 Observer 인터페이스
 *
 * Core 엔진의 상태 변경, 인터락 위반, 로그 메시지 등을 외부에 통지하는
 * Observer 패턴 인터페이스입니다.
 *
 * View DLL(CSeqMonitorFrame)이 이 인터페이스를 구현하여 SequenceEngine에
 * 등록하면, 엔진 내부 이벤트가 발생할 때마다 콜백이 호출됩니다.
 *
 * @note 모든 콜백에 threadId가 포함되어 멀티스레드 디버깅이 가능합니다.
 * @warning 콜백은 워커 스레드에서 호출됩니다. UI 갱신이 필요한 경우
 *          PostMessage 등으로 UI 스레드에 전달해야 합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace YggdraSeq
{

// 전방 선언
class Transition;

/**
 * @class ISequenceObserver
 * @brief 시퀀스 엔진의 이벤트를 수신하는 순수 가상 인터페이스
 *
 * Core와 View 간의 유일한 통신 채널입니다.
 * Core는 View의 존재를 모르며, 이 인터페이스를 통해서만 이벤트를 통지합니다.
 *
 * 구현 시 주의사항:
 * - 콜백 내에서 장시간 블로킹하지 마세요 (엔진 루프를 차단할 수 있음)
 * - 콜백 내에서 엔진 API를 직접 호출하면 데드락이 발생할 수 있습니다
 * - UI 갱신은 반드시 PostMessage 등 비동기 방식으로 수행하세요
 */
class AFX_EXT_CLASS ISequenceObserver
{
public:
    /**
     * @brief 가상 소멸자
     */
    virtual ~ISequenceObserver() = default;

    /**
     * @brief 상태 전이 발생 시 호출
     *
     * 하나의 상태에서 다른 상태로 전이가 성공적으로 완료된 후 호출됩니다.
     * guard 조건과 인터락 체크가 모두 통과한 경우에만 호출됩니다.
     *
     * @param threadId        전이를 수행한 스레드 ID (OS 스레드 ID)
     * @param fromState       출발 상태 이름
     * @param toState         도착 상태 이름
     * @param guardResult     guard 조건 평가 결과 (true = 통과)
     * @param interlockResults 각 인터락의 개별 체크 결과 목록
     */
    virtual void onStateChanged(
        uint32_t threadId,
        const std::string& fromState,
        const std::string& toState,
        bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults) = 0;

    /**
     * @brief 엔진 상태 변경 시 호출
     *
     * 엔진의 실행 상태(Idle, Running, Paused 등)가 변경될 때 호출됩니다.
     * UI에서 상태 표시등(LED 인디케이터) 갱신에 활용합니다.
     *
     * @param threadId  상태 변경을 유발한 스레드 ID
     * @param newStatus 새로운 엔진 상태
     */
    virtual void onEngineStatusChanged(
        uint32_t threadId,
        EngineStatus newStatus) = 0;

    /**
     * @brief 인터락 위반 발생 시 호출
     *
     * 인터락 조건이 false를 반환했을 때 (= 안전하지 않은 상태) 호출됩니다.
     * 심각도에 따라 Warning/Critical/EMO 처리가 이루어진 후 통지됩니다.
     *
     * @param threadId       위반이 발생한 스레드 ID
     * @param interlockName  위반된 인터락 이름
     * @param severity       인터락 심각도
     * @param description    인터락 설명
     * @param currentValue   위반 시점의 현재 값 (문자열 형태)
     * @param thresholdValue 인터락 임계 값 (문자열 형태)
     */
    virtual void onInterlockViolation(
        uint32_t threadId,
        const std::string& interlockName,
        InterlockSeverity severity,
        const std::string& description,
        const std::string& currentValue,
        const std::string& thresholdValue) = 0;

    /**
     * @brief 인터락 체크 완료 시 호출 (정상 포함)
     *
     * 모든 인터락 체크 결과를 통지합니다 (정상/위반 모두).
     * CInterlockPane에서 인터락 현황을 실시간 갱신할 때 사용합니다.
     *
     * @param threadId       체크를 수행한 스레드 ID
     * @param interlockName  체크한 인터락 이름
     * @param currentValue   현재 값 (문자열 형태)
     * @param passed         체크 결과 (true = 안전, false = 위반)
     */
    virtual void onInterlockChecked(
        uint32_t threadId,
        const std::string& interlockName,
        const std::string& currentValue,
        bool passed) = 0;

    /**
     * @brief 로그 메시지 발생 시 호출
     *
     * Logger에서 로그를 기록할 때 동시에 Observer에도 전달됩니다.
     * CLogOutputPane에서 실시간 로그 표시에 활용합니다.
     *
     * @param threadId 로그를 발생시킨 스레드 ID
     * @param level    로그 레벨
     * @param source   로그 발생 소스 (클래스명 또는 모듈명)
     * @param message  로그 메시지 내용
     */
    virtual void onLogMessage(
        uint32_t threadId,
        LogLevel level,
        const std::string& source,
        const std::string& message) = 0;

    /**
     * @brief 에러 발생 시 호출
     *
     * 에러 코드와 함께 에러 상세 설명을 전달합니다.
     * View에서 에러 알림, 상태 바 갱신 등에 활용합니다.
     *
     * @param threadId    에러가 발생한 스레드 ID
     * @param errorCode   에러 코드 (Types.h의 ErrorCode 열거형)
     * @param description 에러 상세 설명
     */
    virtual void onError(
        uint32_t threadId,
        ErrorCode errorCode,
        const std::string& description) = 0;

    /**
     * @brief 워커 스레드 상태 변경 시 호출
     *
     * 스레드의 상태(Running, Waiting, Blocked 등)가 변경될 때 호출됩니다.
     * CThreadMonitorPane에서 스레드 모니터링 및 데드락 감지에 활용합니다.
     *
     * @param threadId   상태가 변경된 스레드 ID
     * @param threadName 스레드 이름 (예: "Engine_Main", "Inspection")
     * @param newState   새로운 스레드 상태
     * @param lockInfo   현재 보유/대기 중인 동기화 객체 정보
     */
    virtual void onThreadStateChanged(
        uint32_t threadId,
        const std::string& threadName,
        ThreadState newState,
        const std::string& lockInfo) = 0;

    /**
     * @brief 브레이크포인트 도달 시 호출
     *
     * 디버그 모드에서 브레이크포인트가 설정된 상태에 진입했을 때 호출됩니다.
     * CSequenceDebugPane에서 스텝 실행/변수 워치를 갱신할 때 사용합니다.
     *
     * @param threadId  브레이크포인트에 도달한 스레드 ID
     * @param stateName 브레이크포인트가 설정된 상태 이름
     * @param params    해당 상태의 현재 파라미터 (key-value)
     */
    virtual void onDebugBreak(
        uint32_t threadId,
        const std::string& stateName,
        const std::map<std::string, ParamValue>& params) = 0;

    /**
     * @brief 전이 조건 평가 완료 시 호출 (성공/실패 모두)
     *
     * 전이 시도 시 guard 조건과 인터락을 평가한 결과를 통지합니다.
     * 전이 성공뿐 아니라 실패한 시도도 포함하여 디버깅에 활용합니다.
     * CTransitionLogPane에서 전이 이력 기록에 사용합니다.
     *
     * @param threadId         평가를 수행한 스레드 ID
     * @param fromState        출발 상태 이름
     * @param toState          도착 상태 이름
     * @param guardResult      guard 조건 평가 결과
     * @param interlockResults 각 인터락의 체크 결과 목록
     */
    virtual void onTransitionEvaluated(
        uint32_t threadId,
        const std::string& fromState,
        const std::string& toState,
        bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults) = 0;
};

} // namespace YggdraSeq
