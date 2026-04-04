/**
 * @file CSeqMonitorFrame.h
 * @brief 도킹 패널 프레임워크의 메인 프레임 클래스
 *
 * CFrameWndEx에서 파생하여 VS Code 스타일의 도킹 레이아웃을 구현합니다.
 * ISequenceObserver를 구현하여 Core 엔진의 이벤트를 수신하고,
 * PostMessage를 통해 각 도킹 패널에 UI 갱신을 전달합니다.
 *
 * VS Code 레이아웃 구조:
 *   좌측 사이드바(아이콘 탭) + 중앙 메인 영역(탭) + 하단 패널(탭)
 *
 * 스레드 안전:
 *   모든 Observer 콜백은 워커 스레드에서 호출됩니다.
 *   콜백 데이터를 스레드 안전한 큐에 저장한 후 PostMessage로 UI 스레드에 전달합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/ISequenceObserver.h"
#include "../YggdraSeqCore/SequenceEngine.h"
#include "CStateListPane.h"
#include "CInterlockPane.h"
#include "CLogOutputPane.h"
#include "CThreadMonitorPane.h"
#include "CSequenceDebugPane.h"
#include "CTransitionLogPane.h"

#include <deque>
#include <mutex>

namespace YggdraSeq
{

// ============================================================================
// Observer 콜백에서 UI 스레드로 전달할 데이터 구조체들
// ============================================================================

/**
 * @struct StateChangedData
 * @brief onStateChanged 콜백 데이터를 UI 스레드로 전달하기 위한 구조체
 */
struct StateChangedData
{
    uint32_t threadId;                              ///< 전이를 수행한 스레드 ID
    std::string fromState;                          ///< 출발 상태 이름
    std::string toState;                            ///< 도착 상태 이름
    bool guardResult;                               ///< guard 조건 평가 결과
    std::vector<InterlockCheckResult> interlockResults;  ///< 인터락 체크 결과 목록
    std::chrono::system_clock::time_point timestamp; ///< 이벤트 발생 시간
};

/**
 * @struct InterlockViolationData
 * @brief onInterlockViolation 콜백 데이터
 */
struct InterlockViolationData
{
    uint32_t threadId;
    std::string interlockName;
    InterlockSeverity severity;
    std::string description;
    std::string currentValue;
    std::string thresholdValue;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct InterlockCheckedData
 * @brief onInterlockChecked 콜백 데이터
 */
struct InterlockCheckedData
{
    uint32_t threadId;
    std::string interlockName;
    std::string currentValue;
    bool passed;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct LogMessageData
 * @brief onLogMessage 콜백 데이터
 */
struct LogMessageData
{
    uint32_t threadId;
    LogLevel level;
    std::string source;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct ErrorData
 * @brief onError 콜백 데이터
 */
struct ErrorData
{
    uint32_t threadId;
    ErrorCode errorCode;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct ThreadStateData
 * @brief onThreadStateChanged 콜백 데이터
 */
struct ThreadStateData
{
    uint32_t threadId;
    std::string threadName;
    ThreadState newState;
    std::string lockInfo;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct DebugBreakData
 * @brief onDebugBreak 콜백 데이터
 */
struct DebugBreakData
{
    uint32_t threadId;
    std::string stateName;
    std::map<std::string, ParamValue> params;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @struct TransitionEvalData
 * @brief onTransitionEvaluated 콜백 데이터
 */
struct TransitionEvalData
{
    uint32_t threadId;
    std::string fromState;
    std::string toState;
    bool guardResult;
    std::vector<InterlockCheckResult> interlockResults;
    std::chrono::system_clock::time_point timestamp;
};

// ============================================================================
// CSeqMonitorFrame 클래스
// ============================================================================

/**
 * @class CSeqMonitorFrame
 * @brief 도킹 패널 프레임워크의 메인 프레임
 *
 * 모든 도킹 패널(6종)을 생성/관리하고, ISequenceObserver를 구현하여
 * Core 엔진의 이벤트를 수신합니다.
 *
 * 이벤트 전달 흐름:
 *   1. Core 엔진 워커 스레드에서 Observer 콜백 호출
 *   2. 콜백 데이터를 스레드 안전한 큐에 push
 *   3. PostMessage로 UI 스레드에 통지
 *   4. UI 스레드에서 메시지 핸들러가 큐에서 데이터를 pop
 *   5. 해당 도킹 패널의 갱신 메서드 호출
 */
class AFX_EXT_CLASS CSeqMonitorFrame : public CWnd, public ISequenceObserver
{
    DECLARE_DYNAMIC(CSeqMonitorFrame)

public:
    CSeqMonitorFrame();
    virtual ~CSeqMonitorFrame();

    // ========================================================================
    // 엔진 연결
    // ========================================================================

    /**
     * @brief 관찰 대상 엔진을 연결
     *
     * 엔진에 Observer로 등록하여 이벤트 수신을 시작합니다.
     * 기존 연결이 있으면 먼저 해제합니다.
     *
     * @param pEngine 관찰할 SequenceEngine 포인터 (소유권 없음)
     */
    void attachEngine(SequenceEngine* pEngine);

    /**
     * @brief 엔진 연결 해제
     *
     * Observer 등록을 해제합니다.
     */
    void detachEngine();

    /**
     * @brief 복수 엔진을 동시에 관찰
     *
     * Inspection + Handler 등 복수 엔진을 모두 Observer로 등록합니다.
     *
     * @param engines 관찰할 엔진 포인터 목록
     */
    void attachEngines(const std::vector<SequenceEngine*>& engines);

    // ========================================================================
    // 패널 접근자
    // ========================================================================

    CStateListPane& getStateListPane() { return m_paneStateList; }
    CInterlockPane& getInterlockPane() { return m_paneInterlock; }
    CLogOutputPane& getLogOutputPane() { return m_paneLog; }
    CThreadMonitorPane& getThreadMonitorPane() { return m_paneThread; }
    CSequenceDebugPane& getSequenceDebugPane() { return m_paneDebug; }
    CTransitionLogPane& getTransitionLogPane() { return m_paneTransition; }

    // ========================================================================
    // ISequenceObserver 구현
    // ========================================================================

    void onStateChanged(uint32_t threadId, const std::string& fromState,
        const std::string& toState, bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults) override;

    void onEngineStatusChanged(uint32_t threadId, EngineStatus newStatus) override;

    void onInterlockViolation(uint32_t threadId, const std::string& interlockName,
        InterlockSeverity severity, const std::string& description,
        const std::string& currentValue, const std::string& thresholdValue) override;

    void onInterlockChecked(uint32_t threadId, const std::string& interlockName,
        const std::string& currentValue, bool passed) override;

    void onLogMessage(uint32_t threadId, LogLevel level,
        const std::string& source, const std::string& message) override;

    void onError(uint32_t threadId, ErrorCode errorCode,
        const std::string& description) override;

    void onThreadStateChanged(uint32_t threadId, const std::string& threadName,
        ThreadState newState, const std::string& lockInfo) override;

    void onDebugBreak(uint32_t threadId, const std::string& stateName,
        const std::map<std::string, ParamValue>& params) override;

    void onTransitionEvaluated(uint32_t threadId, const std::string& fromState,
        const std::string& toState, bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults) override;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();

    // 커스텀 메시지 핸들러 (UI 스레드에서 실행)
    afx_msg LRESULT OnSeqStateChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqEngineStatus(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqInterlockViolation(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqInterlockChecked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqLogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqThreadState(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqDebugBreak(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSeqTransitionEval(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // ========================================================================
    // 탭 패널 생성
    // ========================================================================

    /**
     * @brief CMFCTabCtrl에 모든 패널을 탭으로 생성/등록
     * @return TRUE = 성공
     */
    BOOL createTabPanes();

    // ========================================================================
    // 스레드 안전 데이터 큐 (워커 스레드 -> UI 스레드)
    // ========================================================================

    std::deque<StateChangedData> m_stateChangedQueue;
    std::deque<InterlockViolationData> m_interlockViolationQueue;
    std::deque<InterlockCheckedData> m_interlockCheckedQueue;
    std::deque<LogMessageData> m_logMessageQueue;
    std::deque<ErrorData> m_errorQueue;
    std::deque<ThreadStateData> m_threadStateQueue;
    std::deque<DebugBreakData> m_debugBreakQueue;
    std::deque<TransitionEvalData> m_transitionEvalQueue;

    std::mutex m_queueMutex;    ///< 모든 큐를 보호하는 뮤텍스

    // ========================================================================
    // 탭 컨트롤 및 패널 멤버
    // ========================================================================

    CMFCTabCtrl m_tabCtrl;                  ///< 탭 컨트롤 (모든 패널을 탭으로 관리)
    CStateListPane m_paneStateList;         ///< 상태 목록 패널
    CInterlockPane m_paneInterlock;         ///< 인터락 패널
    CLogOutputPane m_paneLog;               ///< 로그 출력 패널
    CThreadMonitorPane m_paneThread;        ///< 스레드 모니터 패널
    CSequenceDebugPane m_paneDebug;         ///< 시퀀스 디버그 패널
    CTransitionLogPane m_paneTransition;    ///< 전이 이력 패널

    // ========================================================================
    // 엔진 참조
    // ========================================================================

    std::vector<SequenceEngine*> m_engines; ///< 관찰 중인 엔진 목록 (소유권 없음)
};

} // namespace YggdraSeq
