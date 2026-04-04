/**
 * @file SequenceEngine.h
 * @brief 시퀀스 실행의 메인 컨트롤러
 *
 * 상태 머신을 구동하고, 상태 전이를 평가하며, 인터락을 체크하는 핵심 엔진입니다.
 * 워커 스레드에서 비동기로 시퀀스를 실행하며, mutex/critical section으로
 * 공유 자원을 보호합니다.
 *
 * 주요 기능:
 * - run() / pause() / resume() / abort(): 시퀀스 실행 제어
 * - registerState(): 상태 등록
 * - loadRecipe(): 레시피 로드 (Phase 2)
 * - addObserver() / removeObserver(): Observer 패턴으로 외부 통지
 * - 디버그 모드: 스텝 실행, 브레이크포인트
 *
 * 실행 흐름:
 *   run() -> 워커 스레드 시작 -> engineLoop()
 *   engineLoop: currentState->onExecute() -> 전이 평가 -> 전이 실행 -> 반복
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace YggdraSeq
{

// 전방 선언
class IState;
class Transition;
class InterlockManager;
class ISequenceObserver;
class Logger;

/**
 * @class SequenceEngine
 * @brief 시퀀스 실행의 중심 컨트롤러 클래스
 *
 * 상태 머신을 구동하여 반도체 장비의 공정 시퀀스를 제어합니다.
 * 하나의 SequenceEngine은 하나의 워커 스레드에서 시퀀스를 실행합니다.
 * 멀티 시퀀스(Inspection + Handler)는 각각 별도의 SequenceEngine 인스턴스를 생성합니다.
 *
 * 스레드 안전:
 * - 모든 public 메서드는 mutex로 보호됩니다
 * - 상태 변경은 atomic 변수로 관리됩니다
 * - Observer 통지는 워커 스레드에서 수행됩니다
 *
 * @note Core DLL에 MFC UI 코드를 넣지 마세요. UI는 View DLL에서 처리합니다.
 */
class AFX_EXT_CLASS SequenceEngine
{
public:
    /**
     * @brief SequenceEngine 생성자
     *
     * @param engineName 엔진 이름 (로깅 및 스레드 식별용, 예: "Inspection", "Handler")
     */
    explicit SequenceEngine(const std::string& engineName);

    /**
     * @brief 소멸자
     *
     * 워커 스레드가 실행 중이면 abort() 후 join()합니다.
     */
    ~SequenceEngine();

    // 복사/이동 금지 (워커 스레드 소유)
    SequenceEngine(const SequenceEngine&) = delete;
    SequenceEngine& operator=(const SequenceEngine&) = delete;
    SequenceEngine(SequenceEngine&&) = delete;
    SequenceEngine& operator=(SequenceEngine&&) = delete;

    // ========================================================================
    // 시퀀스 실행 제어
    // ========================================================================

    /**
     * @brief 시퀀스 실행 시작
     *
     * 워커 스레드를 생성하고 초기 상태부터 시퀀스를 실행합니다.
     * 이미 실행 중이면 EngineAlreadyRunning 에러를 반환합니다.
     * 등록된 상태가 없거나 초기 상태가 설정되지 않으면 에러를 반환합니다.
     *
     * @return ErrorCode 실행 시작 결과
     */
    ErrorCode run();

    /**
     * @brief 시퀀스 일시 정지
     *
     * 현재 상태의 onExecute() 완료 후 대기 상태로 전환합니다.
     * 실행 중이 아니면 EngineNotRunning 에러를 반환합니다.
     *
     * @return ErrorCode 일시 정지 결과
     */
    ErrorCode pause();

    /**
     * @brief 일시 정지된 시퀀스 재개
     *
     * pause() 상태에서 실행을 계속합니다.
     * Paused 상태가 아니면 에러를 반환합니다.
     *
     * @return ErrorCode 재개 결과
     */
    ErrorCode resume();

    /**
     * @brief 시퀀스 강제 중단
     *
     * 현재 상태의 실행을 중단하고 엔진을 Aborted 상태로 전환합니다.
     * 워커 스레드가 안전하게 종료될 때까지 대기합니다.
     *
     * @return ErrorCode 중단 결과
     */
    ErrorCode abort();

    // ========================================================================
    // 상태 등록
    // ========================================================================

    /**
     * @brief 상태를 엔진에 등록
     *
     * IState 구현체를 엔진에 등록합니다. 상태 이름이 중복되면 에러를 반환합니다.
     * 시퀀스 실행 전에 모든 상태를 등록해야 합니다.
     *
     * @param state 등록할 상태 (소유권 이전, unique_ptr)
     * @return ErrorCode 등록 결과 (Success / StateAlreadyRegistered)
     */
    ErrorCode registerState(std::unique_ptr<IState> state);

    /**
     * @brief 초기 상태 이름을 설정
     *
     * run() 시 이 상태부터 시퀀스가 시작됩니다.
     * registerState()로 등록된 상태의 이름이어야 합니다.
     *
     * @param stateName 초기 상태 이름
     * @return ErrorCode 설정 결과 (Success / StateNotFound)
     */
    ErrorCode setInitialState(const std::string& stateName);

    // ========================================================================
    // 레시피 로드 (Phase 2 준비)
    // ========================================================================

    /**
     * @brief 레시피를 로드 (Phase 2에서 구현)
     *
     * JSON 레시피 파일을 파싱하여 각 상태에 파라미터를 주입합니다.
     *
     * @param recipePath 레시피 JSON 파일 경로
     * @return ErrorCode 로드 결과
     * @note Phase 1에서는 NotImplemented 느낌으로 Success를 반환합니다.
     */
    ErrorCode loadRecipe(const std::string& recipePath);

    // ========================================================================
    // Observer 관리
    // ========================================================================

    /**
     * @brief Observer를 등록
     *
     * 등록된 Observer는 상태 전이, 인터락 위반, 로그 메시지 등의 이벤트를 수신합니다.
     * 최대 MAX_OBSERVER_COUNT개까지 등록할 수 있습니다.
     *
     * @param observer 등록할 Observer (소유권 없음, 관찰만)
     * @note Observer의 수명은 SequenceEngine보다 길어야 합니다.
     */
    void addObserver(ISequenceObserver* observer);

    /**
     * @brief Observer를 제거
     *
     * @param observer 제거할 Observer
     */
    void removeObserver(ISequenceObserver* observer);

    // ========================================================================
    // 인터락 매니저 접근
    // ========================================================================

    /**
     * @brief InterlockManager에 대한 참조를 반환
     *
     * 외부에서 전역 인터락을 등록/제거할 때 사용합니다.
     *
     * @return InterlockManager 참조
     */
    InterlockManager& getInterlockManager();

    /**
     * @brief InterlockManager에 대한 const 참조를 반환
     * @return InterlockManager const 참조
     */
    const InterlockManager& getInterlockManager() const;

    // ========================================================================
    // 디버그 모드
    // ========================================================================

    /**
     * @brief 디버그 모드 활성화/비활성화
     *
     * 디버그 모드가 활성화되면:
     * - 매 상태 전이 전에 대기 (step execution)
     * - 브레이크포인트 설정된 상태에서 자동 pause
     *
     * @param enabled true = 디버그 모드 활성화
     */
    void setDebugMode(bool enabled);

    /**
     * @brief 디버그 모드 여부 반환
     * @return true = 디버그 모드 활성 중
     */
    bool isDebugMode() const;

    /**
     * @brief 브레이크포인트 설정
     *
     * 지정된 상태에 진입할 때 자동으로 pause됩니다.
     *
     * @param stateName 브레이크포인트를 설정할 상태 이름
     */
    void addBreakpoint(const std::string& stateName);

    /**
     * @brief 브레이크포인트 해제
     *
     * @param stateName 브레이크포인트를 해제할 상태 이름
     */
    void removeBreakpoint(const std::string& stateName);

    /**
     * @brief 모든 브레이크포인트 해제
     */
    void clearBreakpoints();

    /**
     * @brief 디버그 모드에서 다음 스텝으로 진행
     *
     * 스텝 실행 모드에서 다음 상태 전이를 1회 수행합니다.
     *
     * @return ErrorCode 스텝 실행 결과
     */
    ErrorCode stepNext();

    // ========================================================================
    // 상태 접근자
    // ========================================================================

    /**
     * @brief 현재 엔진 상태 반환
     * @return 현재 EngineStatus
     */
    EngineStatus getStatus() const;

    /**
     * @brief 현재 실행 중인 상태 이름 반환
     * @return 현재 상태 이름 (실행 중이 아니면 빈 문자열)
     */
    std::string getCurrentStateName() const;

    /**
     * @brief 엔진 이름 반환
     * @return 엔진 이름
     */
    const std::string& getEngineName() const;

    /**
     * @brief 등록된 상태 이름 목록 반환
     * @return 상태 이름 벡터
     */
    std::vector<std::string> getStateNames() const;

    /**
     * @brief 등록된 상태에 대한 포인터를 반환 (읽기 전용)
     * @param stateName 상태 이름
     * @return 상태 포인터 (찾지 못하면 nullptr)
     */
    const IState* getState(const std::string& stateName) const;

    /**
     * @brief Logger에 대한 shared_ptr을 반환
     * @return Logger 공유 포인터
     */
    std::shared_ptr<Logger> getLogger() const;

private:
    // ========================================================================
    // 워커 스레드 실행 로직
    // ========================================================================

    /**
     * @brief 워커 스레드의 메인 루프
     *
     * 시퀀스를 실행하는 핵심 루프입니다.
     * 현재 상태의 onExecute()를 호출하고, 전이를 평가하여 다음 상태로 이동합니다.
     *
     * 루프 동작:
     * 1. pause 상태이면 resume 대기
     * 2. abort 요청이면 루프 종료
     * 3. 전역 인터락 체크 (InterlockManager::checkAll)
     * 4. 현재 상태 onExecute() 호출
     * 5. 전이 평가 (우선순위 순서)
     * 6. 유효한 전이가 있으면 상태 전환 (onExit -> onEntry)
     * 7. End 상태에 도달하면 Complete
     */
    void engineLoop();

    /**
     * @brief 전이를 평가하고 가장 적합한 전이를 선택
     *
     * 현재 상태의 모든 전이를 우선순위 순서로 평가합니다.
     * guard -> interlock 순서로 체크하여 첫 번째 통과한 전이를 반환합니다.
     *
     * @param[out] selectedToState 선택된 전이의 도착 상태 이름
     * @return true = 유효한 전이를 찾음, false = 전이 없음 (현재 상태 유지)
     */
    bool evaluateTransitions(std::string& selectedToState);

    /**
     * @brief 상태 전환을 실행
     *
     * 현재 상태의 onExit()를 호출하고, 다음 상태의 onEntry()를 호출합니다.
     * Observer에 상태 변경을 통지합니다.
     *
     * @param toStateName 전이할 도착 상태 이름
     * @return ErrorCode 전환 결과
     */
    ErrorCode executeTransition(const std::string& toStateName);

    /**
     * @brief 엔진 상태를 변경하고 Observer에 통지
     *
     * @param newStatus 새로운 엔진 상태
     */
    void setStatus(EngineStatus newStatus);

    // ========================================================================
    // Observer 통지 (워커 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 모든 Observer에게 상태 전이를 통지
     */
    void notifyStateChanged(const std::string& fromState,
                            const std::string& toState,
                            bool guardResult,
                            const std::vector<InterlockCheckResult>& interlockResults);

    /**
     * @brief 모든 Observer에게 인터락 위반을 통지
     */
    void notifyInterlockViolation(const InterlockCheckResult& result);

    /**
     * @brief 모든 Observer에게 인터락 체크 결과를 통지
     */
    void notifyInterlockChecked(const InterlockCheckResult& result);

    /**
     * @brief 모든 Observer에게 에러를 통지
     */
    void notifyError(ErrorCode errorCode, const std::string& description);

    /**
     * @brief 모든 Observer에게 스레드 상태 변경을 통지
     */
    void notifyThreadStateChanged(ThreadState newState, const std::string& lockInfo);

    /**
     * @brief 모든 Observer에게 디버그 브레이크를 통지
     */
    void notifyDebugBreak(const std::string& stateName,
                          const std::map<std::string, ParamValue>& params);

    /**
     * @brief 모든 Observer에게 전이 평가 결과를 통지
     */
    void notifyTransitionEvaluated(const std::string& fromState,
                                   const std::string& toState,
                                   bool guardResult,
                                   const std::vector<InterlockCheckResult>& interlockResults);

    // ========================================================================
    // 멤버 변수
    // ========================================================================

    std::string m_engineName;               ///< 엔진 이름 (로깅/스레드 식별용)

    // --- 상태 머신 ---
    std::map<std::string, std::unique_ptr<IState>> m_states;    ///< 등록된 상태 맵 (이름 -> 상태)
    IState* m_currentState;                 ///< 현재 실행 중인 상태 (관찰용 포인터)
    std::string m_initialStateName;         ///< 초기 상태 이름

    // --- 엔진 상태 ---
    std::atomic<EngineStatus> m_status;     ///< 현재 엔진 상태 (atomic으로 스레드 안전)
    std::atomic<bool> m_abortRequested;     ///< abort() 요청 플래그
    std::atomic<bool> m_pauseRequested;     ///< pause() 요청 플래그

    // --- 인터락 ---
    std::unique_ptr<InterlockManager> m_interlockManager;   ///< 전역 인터락 매니저

    // --- Observer ---
    std::vector<ISequenceObserver*> m_observers;    ///< 등록된 Observer 목록 (관찰용)
    std::mutex m_observerMutex;                     ///< Observer 목록 동기화용

    // --- 워커 스레드 ---
    std::unique_ptr<std::thread> m_workerThread;    ///< 워커 스레드
    mutable std::mutex m_engineMutex;                  ///< 엔진 상태 동기화용
    std::condition_variable m_pauseCondition;        ///< pause/resume 동기화용
    std::mutex m_pauseMutex;                         ///< pause condition 보호용

    // --- 디버그 모드 ---
    std::atomic<bool> m_debugMode;                  ///< 디버그 모드 활성 여부
    std::set<std::string> m_breakpoints;            ///< 브레이크포인트 설정된 상태 이름 집합
    std::mutex m_debugMutex;                        ///< 디버그 설정 동기화용
    std::atomic<bool> m_stepRequested;              ///< 스텝 실행 요청 플래그
    std::condition_variable m_stepCondition;         ///< 스텝 실행 동기화용
    std::mutex m_stepMutex;                          ///< step condition 보호용

    // --- 로거 ---
    std::shared_ptr<Logger> m_logger;               ///< 로거 인스턴스
};

} // namespace YggdraSeq
