/**
 * @file CSeqMonitorFrame.cpp
 * @brief 도킹 패널 프레임워크 메인 프레임 구현
 *
 * ISequenceObserver를 구현하여 Core 엔진 이벤트를 수신하고,
 * PostMessage를 통해 UI 스레드의 각 도킹 패널에 갱신을 전달합니다.
 *
 * 이벤트 전달 패턴:
 *   워커 스레드 콜백 -> 큐에 push + PostMessage -> UI 스레드 핸들러 -> 패널 갱신
 */

#include "pch.h"
#include "CSeqMonitorFrame.h"

namespace YggdraSeq
{

// ============================================================================
// MFC 동적 생성 매크로
// ============================================================================

IMPLEMENT_DYNAMIC(CSeqMonitorFrame, CWnd)

// ============================================================================
// 메시지 맵
// ============================================================================

BEGIN_MESSAGE_MAP(CSeqMonitorFrame, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_SEQ_STATE_CHANGED, &CSeqMonitorFrame::OnSeqStateChanged)
    ON_MESSAGE(WM_SEQ_ENGINE_STATUS, &CSeqMonitorFrame::OnSeqEngineStatus)
    ON_MESSAGE(WM_SEQ_INTERLOCK_VIOLATION, &CSeqMonitorFrame::OnSeqInterlockViolation)
    ON_MESSAGE(WM_SEQ_INTERLOCK_CHECKED, &CSeqMonitorFrame::OnSeqInterlockChecked)
    ON_MESSAGE(WM_SEQ_LOG_MESSAGE, &CSeqMonitorFrame::OnSeqLogMessage)
    ON_MESSAGE(WM_SEQ_ERROR, &CSeqMonitorFrame::OnSeqError)
    ON_MESSAGE(WM_SEQ_THREAD_STATE, &CSeqMonitorFrame::OnSeqThreadState)
    ON_MESSAGE(WM_SEQ_DEBUG_BREAK, &CSeqMonitorFrame::OnSeqDebugBreak)
    ON_MESSAGE(WM_SEQ_TRANSITION_EVAL, &CSeqMonitorFrame::OnSeqTransitionEval)
END_MESSAGE_MAP()

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

CSeqMonitorFrame::CSeqMonitorFrame()
{
}

CSeqMonitorFrame::~CSeqMonitorFrame()
{
    // 모든 엔진에서 Observer 등록 해제
    detachEngine();
}

// ============================================================================
// 엔진 연결 / 해제
// ============================================================================

void CSeqMonitorFrame::attachEngine(SequenceEngine* pEngine)
{
    if (pEngine == nullptr)
        return;

    // 기존 연결이 있으면 먼저 해제
    detachEngine();

    m_engines.push_back(pEngine);
    pEngine->addObserver(this);
}

void CSeqMonitorFrame::detachEngine()
{
    for (auto* pEngine : m_engines)
    {
        if (pEngine != nullptr)
        {
            pEngine->removeObserver(this);
        }
    }
    m_engines.clear();
}

void CSeqMonitorFrame::attachEngines(const std::vector<SequenceEngine*>& engines)
{
    // 기존 연결 해제
    detachEngine();

    for (auto* pEngine : engines)
    {
        if (pEngine != nullptr)
        {
            m_engines.push_back(pEngine);
            pEngine->addObserver(this);
        }
    }
}

// ============================================================================
// CWnd 생성
// ============================================================================

int CSeqMonitorFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // CMFCTabCtrl 생성 (하단 탭, OneNote 스타일)
    CRect rect;
    GetClientRect(rect);
    if (!m_tabCtrl.Create(CMFCTabCtrl::STYLE_3D_ONENOTE, rect, this, 1,
        CMFCTabCtrl::LOCATION_BOTTOM))
    {
        TRACE0("Failed to create CMFCTabCtrl\n");
        return -1;
    }
    m_tabCtrl.SetFlatFrame(TRUE);
    m_tabCtrl.AutoDestroyWindow(FALSE);

    // 탭 패널 생성 및 등록
    if (!createTabPanes())
        return -1;

    return 0;
}

void CSeqMonitorFrame::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    // CMFCTabCtrl을 클라이언트 영역에 맞게 리사이즈
    if (m_tabCtrl.GetSafeHwnd() != nullptr)
    {
        m_tabCtrl.MoveWindow(0, 0, cx, cy);
    }
}

void CSeqMonitorFrame::OnDestroy()
{
    // 엔진 연결 해제
    detachEngine();

    CWnd::OnDestroy();
}

// ============================================================================
// 탭 패널 생성
// ============================================================================

BOOL CSeqMonitorFrame::createTabPanes()
{
    CRect rect(0, 0, 100, 100);
    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    // 상태 목록 패널
    if (!m_paneStateList.Create(nullptr, _T("State List"), dwStyle,
        rect, this, IDR_PANE_STATE_LIST))
    {
        TRACE0("Failed to create State List Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneStateList, _T("State List"));

    // 인터락 패널
    if (!m_paneInterlock.Create(nullptr, _T("Interlock"), dwStyle,
        rect, this, IDR_PANE_INTERLOCK))
    {
        TRACE0("Failed to create Interlock Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneInterlock, _T("Interlock"));

    // 시퀀스 디버그 패널
    if (!m_paneDebug.Create(nullptr, _T("Sequence Debug"), dwStyle,
        rect, this, IDR_PANE_SEQUENCE_DEBUG))
    {
        TRACE0("Failed to create Sequence Debug Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneDebug, _T("Sequence Debug"));

    // 로그 출력 패널
    if (!m_paneLog.Create(nullptr, _T("Log Output"), dwStyle,
        rect, this, IDR_PANE_LOG_OUTPUT))
    {
        TRACE0("Failed to create Log Output Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneLog, _T("Log Output"));

    // 전이 이력 패널
    if (!m_paneTransition.Create(nullptr, _T("Transition Log"), dwStyle,
        rect, this, IDR_PANE_TRANSITION_LOG))
    {
        TRACE0("Failed to create Transition Log Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneTransition, _T("Transition Log"));

    // 스레드 모니터 패널
    if (!m_paneThread.Create(nullptr, _T("Thread Monitor"), dwStyle,
        rect, this, IDR_PANE_THREAD_MONITOR))
    {
        TRACE0("Failed to create Thread Monitor Pane\n");
        return FALSE;
    }
    m_tabCtrl.AddTab(&m_paneThread, _T("Thread Monitor"));

    return TRUE;
}

// ============================================================================
// ISequenceObserver 구현 (워커 스레드에서 호출됨)
// 모든 콜백은 데이터를 큐에 push 후 PostMessage로 UI 스레드에 통지
// ============================================================================

void CSeqMonitorFrame::onStateChanged(
    uint32_t threadId, const std::string& fromState, const std::string& toState,
    bool guardResult, const std::vector<InterlockCheckResult>& interlockResults)
{
    StateChangedData data;
    data.threadId = threadId;
    data.fromState = fromState;
    data.toState = toState;
    data.guardResult = guardResult;
    data.interlockResults = interlockResults;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stateChangedQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_STATE_CHANGED, 0, 0);
}

void CSeqMonitorFrame::onEngineStatusChanged(uint32_t threadId, EngineStatus newStatus)
{
    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_ENGINE_STATUS, static_cast<WPARAM>(newStatus), static_cast<LPARAM>(threadId));
}

void CSeqMonitorFrame::onInterlockViolation(
    uint32_t threadId, const std::string& interlockName,
    InterlockSeverity severity, const std::string& description,
    const std::string& currentValue, const std::string& thresholdValue)
{
    InterlockViolationData data;
    data.threadId = threadId;
    data.interlockName = interlockName;
    data.severity = severity;
    data.description = description;
    data.currentValue = currentValue;
    data.thresholdValue = thresholdValue;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_interlockViolationQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_INTERLOCK_VIOLATION, 0, 0);
}

void CSeqMonitorFrame::onInterlockChecked(
    uint32_t threadId, const std::string& interlockName,
    const std::string& currentValue, bool passed)
{
    InterlockCheckedData data;
    data.threadId = threadId;
    data.interlockName = interlockName;
    data.currentValue = currentValue;
    data.passed = passed;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_interlockCheckedQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_INTERLOCK_CHECKED, 0, 0);
}

void CSeqMonitorFrame::onLogMessage(
    uint32_t threadId, LogLevel level,
    const std::string& source, const std::string& message)
{
    LogMessageData data;
    data.threadId = threadId;
    data.level = level;
    data.source = source;
    data.message = message;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_logMessageQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_LOG_MESSAGE, 0, 0);
}

void CSeqMonitorFrame::onError(
    uint32_t threadId, ErrorCode errorCode, const std::string& description)
{
    ErrorData data;
    data.threadId = threadId;
    data.errorCode = errorCode;
    data.description = description;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_errorQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_ERROR, 0, 0);
}

void CSeqMonitorFrame::onThreadStateChanged(
    uint32_t threadId, const std::string& threadName,
    ThreadState newState, const std::string& lockInfo)
{
    ThreadStateData data;
    data.threadId = threadId;
    data.threadName = threadName;
    data.newState = newState;
    data.lockInfo = lockInfo;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_threadStateQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_THREAD_STATE, 0, 0);
}

void CSeqMonitorFrame::onDebugBreak(
    uint32_t threadId, const std::string& stateName,
    const std::map<std::string, ParamValue>& params)
{
    DebugBreakData data;
    data.threadId = threadId;
    data.stateName = stateName;
    data.params = params;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_debugBreakQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_DEBUG_BREAK, 0, 0);
}

void CSeqMonitorFrame::onTransitionEvaluated(
    uint32_t threadId, const std::string& fromState, const std::string& toState,
    bool guardResult, const std::vector<InterlockCheckResult>& interlockResults)
{
    TransitionEvalData data;
    data.threadId = threadId;
    data.fromState = fromState;
    data.toState = toState;
    data.guardResult = guardResult;
    data.interlockResults = interlockResults;
    data.timestamp = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_transitionEvalQueue.push_back(std::move(data));
    }

    if (GetSafeHwnd() != nullptr)
        PostMessage(WM_SEQ_TRANSITION_EVAL, 0, 0);
}

// ============================================================================
// UI 스레드 메시지 핸들러
// 큐에서 데이터를 꺼내 해당 패널에 전달
// ============================================================================

LRESULT CSeqMonitorFrame::OnSeqStateChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<StateChangedData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_stateChangedQueue);
    }

    for (const auto& data : batch)
    {
        m_paneStateList.updateStateChange(data.threadId, data.fromState,
            data.toState, data.timestamp);
        m_paneTransition.addTransitionRecord(data.threadId, data.fromState,
            data.toState, data.guardResult, data.interlockResults, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqEngineStatus(WPARAM wParam, LPARAM lParam)
{
    auto status = static_cast<EngineStatus>(wParam);
    auto threadId = static_cast<uint32_t>(lParam);

    m_paneStateList.updateEngineStatus(threadId, status);
    m_paneDebug.updateEngineStatus(threadId, status);
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqInterlockViolation(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<InterlockViolationData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_interlockViolationQueue);
    }

    for (const auto& data : batch)
    {
        m_paneInterlock.updateInterlockViolation(data.threadId, data.interlockName,
            data.severity, data.currentValue, data.thresholdValue, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqInterlockChecked(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<InterlockCheckedData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_interlockCheckedQueue);
    }

    for (const auto& data : batch)
    {
        m_paneInterlock.updateInterlockStatus(data.threadId, data.interlockName,
            data.currentValue, data.passed, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqLogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<LogMessageData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_logMessageQueue);
    }

    for (const auto& data : batch)
    {
        m_paneLog.addLogEntry(data.threadId, data.level, data.source,
            data.message, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqError(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<ErrorData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_errorQueue);
    }

    for (const auto& data : batch)
    {
        m_paneLog.addLogEntry(data.threadId, LogLevel::Error, "Error",
            data.description, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqThreadState(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<ThreadStateData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_threadStateQueue);
    }

    for (const auto& data : batch)
    {
        m_paneThread.updateThreadState(data.threadId, data.threadName,
            data.newState, data.lockInfo, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqDebugBreak(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<DebugBreakData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_debugBreakQueue);
    }

    for (const auto& data : batch)
    {
        m_paneDebug.updateDebugBreak(data.threadId, data.stateName, data.params, data.timestamp);
    }
    return 0;
}

LRESULT CSeqMonitorFrame::OnSeqTransitionEval(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::deque<TransitionEvalData> batch;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        batch.swap(m_transitionEvalQueue);
    }

    for (const auto& data : batch)
    {
        m_paneDebug.updateTransitionConditions(data.threadId, data.fromState,
            data.toState, data.guardResult, data.interlockResults);
        m_paneTransition.addTransitionRecord(data.threadId, data.fromState,
            data.toState, data.guardResult, data.interlockResults, data.timestamp);
    }
    return 0;
}

} // namespace YggdraSeq
