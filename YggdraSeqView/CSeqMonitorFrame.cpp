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

IMPLEMENT_DYNCREATE(CSeqMonitorFrame, CFrameWndEx)

// ============================================================================
// 메시지 맵
// ============================================================================

BEGIN_MESSAGE_MAP(CSeqMonitorFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_WM_CLOSE()
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
// MFC 프레임 생성
// ============================================================================

BOOL CSeqMonitorFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    cs.lpszClass = AfxRegisterWndClass(0);
    return TRUE;
}

BOOL CSeqMonitorFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    return CFrameWndEx::OnCreateClient(lpcs, pContext);
}

int CSeqMonitorFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 다크 테마 비주얼 매니저 설정
    setupVisualManager();

    // 전 방향 도킹 활성화
    EnableDocking(CBRS_ALIGN_ANY);

    // 도킹 패널 생성 및 레이아웃 배치
    if (!createDockingPanes())
        return -1;

    // 저장된 도킹 레이아웃 복원 시도
    loadDockingLayout();

    return 0;
}

void CSeqMonitorFrame::OnClose()
{
    // 도킹 레이아웃 저장
    saveDockingLayout();

    // 엔진 연결 해제
    detachEngine();

    CFrameWndEx::OnClose();
}

// ============================================================================
// 도킹 패널 생성 및 레이아웃
// ============================================================================

BOOL CSeqMonitorFrame::createDockingPanes()
{
    // --- 중앙 메인 탭에 배치할 패널들 (State/Interlock/Debug) ---

    // 상태 목록 패널 (좌측 상단 영역)
    if (!m_paneStateList.Create(_T("State List"), this, CRect(0, 0, 300, 200),
        TRUE, IDR_PANE_STATE_LIST,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create State List Pane\n");
        return FALSE;
    }
    m_paneStateList.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_paneStateList, AFX_IDW_DOCKBAR_RIGHT);

    // 인터락 패널 (State List와 탭 그룹)
    if (!m_paneInterlock.Create(_T("Interlock"), this, CRect(0, 0, 300, 200),
        TRUE, IDR_PANE_INTERLOCK,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create Interlock Pane\n");
        return FALSE;
    }
    m_paneInterlock.EnableDocking(CBRS_ALIGN_ANY);
    m_paneInterlock.AttachToTabWnd(&m_paneStateList, DM_SHOW, TRUE);

    // 시퀀스 디버그 패널 (State List와 탭 그룹)
    if (!m_paneDebug.Create(_T("Sequence Debug"), this, CRect(0, 0, 300, 200),
        TRUE, IDR_PANE_SEQUENCE_DEBUG,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create Sequence Debug Pane\n");
        return FALSE;
    }
    m_paneDebug.EnableDocking(CBRS_ALIGN_ANY);
    m_paneDebug.AttachToTabWnd(&m_paneStateList, DM_SHOW, TRUE);

    // --- 하단 패널 탭 (Log/Transition/Thread) ---

    // 로그 출력 패널 (하단 영역)
    if (!m_paneLog.Create(_T("Log Output"), this, CRect(0, 0, 200, 150),
        TRUE, IDR_PANE_LOG_OUTPUT,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create Log Output Pane\n");
        return FALSE;
    }
    m_paneLog.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_paneLog, AFX_IDW_DOCKBAR_BOTTOM);

    // 전이 이력 패널 (Log와 탭 그룹)
    if (!m_paneTransition.Create(_T("Transition Log"), this, CRect(0, 0, 200, 150),
        TRUE, IDR_PANE_TRANSITION_LOG,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create Transition Log Pane\n");
        return FALSE;
    }
    m_paneTransition.EnableDocking(CBRS_ALIGN_ANY);
    m_paneTransition.AttachToTabWnd(&m_paneLog, DM_SHOW, TRUE);

    // 스레드 모니터 패널 (Log와 탭 그룹)
    if (!m_paneThread.Create(_T("Thread Monitor"), this, CRect(0, 0, 200, 150),
        TRUE, IDR_PANE_THREAD_MONITOR,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM,
        AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE))
    {
        TRACE0("Failed to create Thread Monitor Pane\n");
        return FALSE;
    }
    m_paneThread.EnableDocking(CBRS_ALIGN_ANY);
    m_paneThread.AttachToTabWnd(&m_paneLog, DM_SHOW, TRUE);

    return TRUE;
}

void CSeqMonitorFrame::setupVisualManager()
{
    // VS 2012 다크 테마 사용 (VS Code 스타일에 가장 가까운 MFC 기본 제공 테마)
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
    CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
    CDockingManager::SetDockingMode(DT_SMART);
}

void CSeqMonitorFrame::loadDockingLayout()
{
    // 레지스트리에서 이전 도킹 레이아웃 복원
    LoadBarState(_T("YggdraSeqView_DockingLayout"));
}

void CSeqMonitorFrame::saveDockingLayout()
{
    // 현재 도킹 레이아웃을 레지스트리에 저장
    SaveBarState(_T("YggdraSeqView_DockingLayout"));
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
