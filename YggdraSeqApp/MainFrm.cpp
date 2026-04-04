/**
 * @file MainFrm.cpp
 * @brief CFrameWndEx 메인 프레임 구현
 *
 * VS Code 스타일 레이아웃:
 *   +---+------------------------------------------+
 *   | A | [Command ToolBar]                        |
 *   | c |------------------------------------------|
 *   | t | [Process Mimic] | [Sensor Data]          |
 *   | i |                 |                        |
 *   | v |                 |                        |
 *   | i |==========================================|
 *   | t | [Log] [Transition] [Thread] (도킹 패널)  |
 *   | y |                                          |
 *   +---+------------------------------------------+
 *   |         Status Bar                           |
 *   +----------------------------------------------+
 *
 * View DLL의 도킹 패널은 CSeqMonitorFrame이 아닌 이 메인 프레임에 직접 배치합니다.
 * CSeqMonitorFrame의 ISequenceObserver 기능만 사용합니다.
 */

#include "pch.h"
#include "MainFrm.h"
#include "InspectionSequence.h"
#include "HandlerSequence.h"
#include "../YggdraSeqCore/Interlock.h"
#include "../YggdraSeqCore/InterlockManager.h"
#include "../YggdraSeqCore/Transition.h"

namespace YggdraSeq
{

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_COMMAND(ID_SEQ_RUN, &CMainFrame::OnSeqRun)
    ON_COMMAND(ID_SEQ_PAUSE, &CMainFrame::OnSeqPause)
    ON_COMMAND(ID_SEQ_STOP, &CMainFrame::OnSeqStop)
    ON_COMMAND(ID_SEQ_EMO, &CMainFrame::OnSeqEmo)
    ON_COMMAND(ID_SEQ_MODE_AUTO, &CMainFrame::OnSeqModeAuto)
    ON_COMMAND(ID_SEQ_MODE_MANUAL, &CMainFrame::OnSeqModeManual)
    ON_COMMAND(ID_SEQ_MODE_DEBUG, &CMainFrame::OnSeqModeDebug)
    ON_UPDATE_COMMAND_UI(ID_SEQ_RUN, &CMainFrame::OnUpdateSeqRun)
    ON_UPDATE_COMMAND_UI(ID_SEQ_PAUSE, &CMainFrame::OnUpdateSeqPause)
    ON_UPDATE_COMMAND_UI(ID_SEQ_STOP, &CMainFrame::OnUpdateSeqStop)
    ON_MESSAGE(WM_APP_HARDWARE_UPDATED, &CMainFrame::OnHardwareUpdated)
END_MESSAGE_MAP()

// 상태 바 인디케이터 정의
static UINT indicators[] =
{
    ID_SEPARATOR,
    ID_STATUSBAR_MODE,
    ID_STATUSBAR_INSPECT,
    ID_STATUSBAR_HANDLER,
    ID_STATUSBAR_THREADS,
    ID_STATUSBAR_ERRORS,
    ID_STATUSBAR_INTERLOCKS,
};

CMainFrame::CMainFrame()
    : m_pMonitorFrame(nullptr)
    , m_pInspectState(nullptr)
    , m_sequencesInitialized(false)
{
}

CMainFrame::~CMainFrame()
{
    // 모니터 프레임 정리 (Observer 등록 해제)
    if (m_pMonitorFrame != nullptr)
    {
        m_pMonitorFrame->detachEngine();
    }
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    cs.lpszClass = AfxRegisterWndClass(0);

    // 초기 창 크기
    cs.cx = 1400;
    cs.cy = 900;

    return TRUE;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* /*pContext*/)
{
    return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 다크 테마 비주얼 매니저
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
    CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
    CDockingManager::SetDockingMode(DT_SMART);
    EnableDocking(CBRS_ALIGN_ANY);

    // 상단 명령 버튼 바 (CWnd - 도킹 없이 고정 배치)
    m_commandToolBar.CreateToolBar(this);

    // 좌측 Activity Bar (CWnd - 도킹 없이 고정 배치)
    CRect barRect(0, 0, CActivityBar::BAR_WIDTH, 600);
    m_activityBar.CreateBar(this, barRect);
    m_activityBar.setModeChangeCallback([this](ActivityMode mode) {
        // 모드 전환 시 메인 뷰 전환 (향후 확장)
        Invalidate();
    });

    // 미믹 다이어그램 (메인 영역)
    m_mimicView.CreateView(this, CRect(0, 0, 100, 100));

    // 센서 데이터 테이블 (메인 영역)
    m_sensorView.CreateView(this, CRect(0, 0, 100, 100));

    // 상태 바
    m_statusBar.Create(this);
    m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
    m_statusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);
    m_statusBar.SetPaneInfo(1, ID_STATUSBAR_MODE, SBPS_NORMAL, 80);
    m_statusBar.SetPaneInfo(2, ID_STATUSBAR_INSPECT, SBPS_NORMAL, 120);
    m_statusBar.SetPaneInfo(3, ID_STATUSBAR_HANDLER, SBPS_NORMAL, 120);
    m_statusBar.SetPaneInfo(4, ID_STATUSBAR_THREADS, SBPS_NORMAL, 80);
    m_statusBar.SetPaneInfo(5, ID_STATUSBAR_ERRORS, SBPS_NORMAL, 80);
    m_statusBar.SetPaneInfo(6, ID_STATUSBAR_INTERLOCKS, SBPS_NORMAL, 100);

    // View DLL 모니터 프레임 생성 (탭 패널 호스트)
    m_pMonitorFrame = new CSeqMonitorFrame();
    m_pMonitorFrame->Create(AfxRegisterWndClass(0), _T("Monitor"),
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CRect(0, 0, 100, 200), this, 0);

    // 시퀀스 엔진 초기화
    initializeSequences();

    // 상태 바 갱신 타이머 (500ms)
    SetTimer(IDT_STATUSBAR_REFRESH, 500, nullptr);

    return 0;
}

void CMainFrame::OnClose()
{
    // 실행 중인 시퀀스 중단
    if (m_pInspectionEngine)
        m_pInspectionEngine->abort();
    if (m_pHandlerEngine)
        m_pHandlerEngine->abort();

    // 타이머 정리
    KillTimer(IDT_STATUSBAR_REFRESH);

    CFrameWndEx::OnClose();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWndEx::OnSize(nType, cx, cy);
    layoutChildren();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_STATUSBAR_REFRESH)
    {
        updateStatusBar();

        // 미믹 다이어그램에 현재 상태 전달
        if (m_pInspectionEngine && m_pHandlerEngine)
        {
            m_mimicView.setActiveStates(
                m_pInspectionEngine->getCurrentStateName(),
                m_pHandlerEngine->getCurrentStateName());
        }
    }

    CFrameWndEx::OnTimer(nIDEvent);
}

void CMainFrame::layoutChildren()
{
    CRect clientRect;
    GetClientRect(clientRect);

    if (clientRect.IsRectEmpty())
        return;

    // 사용 가능한 클라이언트 영역 계산 (메뉴/상태바 제외)
    CRect availRect;
    RepositionBars(0, 0xFFFF, AFX_IDW_PANE_FIRST, CWnd::reposQuery, &availRect);

    int activityBarWidth = CActivityBar::BAR_WIDTH;
    int commandBarHeight = COMMAND_BAR_HEIGHT;
    int monitorHeight = availRect.Height() / 3;  // 하단 1/3을 모니터에 할당

    // Activity Bar (좌측 고정)
    if (m_activityBar.GetSafeHwnd() != nullptr)
    {
        m_activityBar.MoveWindow(availRect.left, availRect.top,
            activityBarWidth, availRect.Height());
    }

    int mainLeft = availRect.left + activityBarWidth;
    int mainWidth = availRect.Width() - activityBarWidth;

    // 상단 명령 버튼 바
    if (m_commandToolBar.GetSafeHwnd() != nullptr)
    {
        m_commandToolBar.MoveWindow(mainLeft, availRect.top,
            mainWidth, commandBarHeight);
    }

    int mainTop = availRect.top + commandBarHeight;
    int mainHeight = availRect.Height() - commandBarHeight - monitorHeight;

    // 미믹 다이어그램 (좌측 60%)
    int mimicWidth = mainWidth * 6 / 10;
    if (m_mimicView.GetSafeHwnd() != nullptr)
    {
        m_mimicView.MoveWindow(mainLeft, mainTop, mimicWidth, mainHeight);
    }

    // 센서 데이터 (우측 40%)
    if (m_sensorView.GetSafeHwnd() != nullptr)
    {
        m_sensorView.MoveWindow(mainLeft + mimicWidth, mainTop,
            mainWidth - mimicWidth, mainHeight);
    }

    // View DLL 모니터 프레임 (하단)
    if (m_pMonitorFrame != nullptr && m_pMonitorFrame->GetSafeHwnd() != nullptr)
    {
        m_pMonitorFrame->MoveWindow(mainLeft, mainTop + mainHeight,
            mainWidth, monitorHeight);
    }
}

// ============================================================================
// 시퀀스 초기화
// ============================================================================

void CMainFrame::initializeSequences()
{
    // 가상 하드웨어 생성
    m_pHardware = std::make_unique<SimulatedHardware>();
    m_pHardware->setNotifyWindow(GetSafeHwnd());

    // 미믹 뷰에 하드웨어 연결
    m_mimicView.setHardware(m_pHardware.get());
    m_sensorView.setHardware(m_pHardware.get());

    // --- Inspection Engine (Thread-A) ---
    m_pInspectionEngine = std::make_unique<SequenceEngine>("Inspection");

    // Inspection 상태 생성
    auto pInspIdle = std::make_unique<InspIdleState>(m_pHardware.get());
    auto pLoad = std::make_unique<LoadState>(m_pHardware.get());
    auto pAlign = std::make_unique<AlignState>(m_pHardware.get());
    auto pCapture = std::make_unique<CaptureState>(m_pHardware.get());
    auto pInspect = std::make_unique<InspectState>(m_pHardware.get());
    m_pInspectState = pInspect.get();  // JudgeState에서 참조
    auto pJudge = std::make_unique<JudgeState>(m_pHardware.get(), m_pInspectState);

    // Inspection 전이 설정
    // InspIdle -> Load (웨이퍼가 있으면)
    auto tIdleToLoad = std::make_unique<Transition>("InspIdle", "Load");
    tIdleToLoad->setGuard([hw = m_pHardware.get()]() { return hw->getWaferCount() > 0; });
    pInspIdle->addTransition(std::move(tIdleToLoad));

    // Load -> Align
    auto tLoadToAlign = std::make_unique<Transition>("Load", "Align");
    tLoadToAlign->setGuard([p = pLoad.get()]() { return p->getParams().empty(); });  // 간단한 가드
    pLoad->addTransition(std::move(tLoadToAlign));

    // Align -> Capture (스테이지 안정 시)
    auto tAlignToCapture = std::make_unique<Transition>("Align", "Capture");
    tAlignToCapture->setGuard([hw = m_pHardware.get()]() { return hw->isStageStable(); });
    pAlign->addTransition(std::move(tAlignToCapture));

    // Capture -> Inspect
    auto tCaptureToInspect = std::make_unique<Transition>("Capture", "Inspect");
    pCapture->addTransition(std::move(tCaptureToInspect));

    // Inspect -> Judge
    auto tInspectToJudge = std::make_unique<Transition>("Inspect", "Judge");
    pInspect->addTransition(std::move(tInspectToJudge));

    // Judge -> InspIdle
    auto tJudgeToIdle = std::make_unique<Transition>("Judge", "InspIdle");
    pJudge->addTransition(std::move(tJudgeToIdle));

    // 상태 등록
    m_pInspectionEngine->registerState(std::move(pInspIdle));
    m_pInspectionEngine->registerState(std::move(pLoad));
    m_pInspectionEngine->registerState(std::move(pAlign));
    m_pInspectionEngine->registerState(std::move(pCapture));
    m_pInspectionEngine->registerState(std::move(pInspect));
    m_pInspectionEngine->registerState(std::move(pJudge));
    m_pInspectionEngine->setInitialState("InspIdle");

    // --- Handler Engine (Thread-B) ---
    m_pHandlerEngine = std::make_unique<SequenceEngine>("Handler");

    auto pHandlerIdle = std::make_unique<HandlerIdleState>(m_pHardware.get());
    auto pPick = std::make_unique<PickState>(m_pHardware.get());
    auto pMove = std::make_unique<MoveState>(m_pHardware.get());
    auto pPlace = std::make_unique<PlaceState>(m_pHardware.get());
    auto pReturn = std::make_unique<ReturnState>(m_pHardware.get());

    // Handler 전이 설정
    // HandlerIdle -> Pick
    auto tIdleToPick = std::make_unique<Transition>("HandlerIdle", "Pick");
    tIdleToPick->setGuard([hw = m_pHardware.get()]() {
        // 검사 완료 후 웨이퍼가 스테이지에 있을 때 (간단 조건)
        return (hw->getGoodCount() + hw->getNgCount()) > 0;
    });
    pHandlerIdle->addTransition(std::move(tIdleToPick));

    // Pick -> Move
    auto tPickToMove = std::make_unique<Transition>("Pick", "Move");
    pPick->addTransition(std::move(tPickToMove));

    // Move -> Place
    auto tMoveToPlace = std::make_unique<Transition>("Move", "Place");
    pMove->addTransition(std::move(tMoveToPlace));

    // Place -> Return
    auto tPlaceToReturn = std::make_unique<Transition>("Place", "Return");
    pPlace->addTransition(std::move(tPlaceToReturn));

    // Return -> HandlerIdle
    auto tReturnToIdle = std::make_unique<Transition>("Return", "HandlerIdle");
    pReturn->addTransition(std::move(tReturnToIdle));

    // 상태 등록
    m_pHandlerEngine->registerState(std::move(pHandlerIdle));
    m_pHandlerEngine->registerState(std::move(pPick));
    m_pHandlerEngine->registerState(std::move(pMove));
    m_pHandlerEngine->registerState(std::move(pPlace));
    m_pHandlerEngine->registerState(std::move(pReturn));
    m_pHandlerEngine->setInitialState("HandlerIdle");

    // 인터락 설정
    setupInterlocks();

    // View DLL 모니터 프레임에 엔진 연결
    if (m_pMonitorFrame != nullptr)
    {
        std::vector<SequenceEngine*> engines = {
            m_pInspectionEngine.get(),
            m_pHandlerEngine.get()
        };
        m_pMonitorFrame->attachEngines(engines);
    }

    m_sequencesInitialized = true;
}

void CMainFrame::setupInterlocks()
{
    // --- Inspection 엔진 인터락 ---
    {
        auto& mgr = m_pInspectionEngine->getInterlockManager();

        // 로봇 팔이 검사 영역 밖에 있어야 촬영 가능
        auto armInterlock = std::make_unique<Interlock>(
            "ArmOutOfInspectArea",
            [hw = m_pHardware.get()]() { return hw->isArmOutOfInspectArea(); },
            InterlockSeverity::Critical,
            "Robot arm must be outside inspection area during capture"
        );
        armInterlock->setThresholdValue("X >= 200mm");
        mgr.addInterlock(std::move(armInterlock));
    }

    // --- Handler 엔진 인터락 ---
    {
        auto& mgr = m_pHandlerEngine->getInterlockManager();

        // 스테이지가 안정 위치에 있어야 웨이퍼 픽업 가능
        auto stageInterlock = std::make_unique<Interlock>(
            "StageStable",
            [hw = m_pHardware.get()]() { return hw->isStageStable(); },
            InterlockSeverity::Warning,
            "Stage must be stable for wafer pickup"
        );
        stageInterlock->setThresholdValue("X=125, Y=80, Theta=0");
        mgr.addInterlock(std::move(stageInterlock));
    }
}

// ============================================================================
// 시퀀스 제어 명령 핸들러
// ============================================================================

void CMainFrame::OnSeqRun()
{
    if (!m_sequencesInitialized)
        return;

    // 양 엔진 동시 시작
    m_pInspectionEngine->run();
    m_pHandlerEngine->run();
}

void CMainFrame::OnSeqPause()
{
    if (m_pInspectionEngine)
        m_pInspectionEngine->pause();
    if (m_pHandlerEngine)
        m_pHandlerEngine->pause();
}

void CMainFrame::OnSeqStop()
{
    if (m_pInspectionEngine)
        m_pInspectionEngine->abort();
    if (m_pHandlerEngine)
        m_pHandlerEngine->abort();
}

void CMainFrame::OnSeqEmo()
{
    // EMO: 즉시 모든 엔진 중단
    if (m_pInspectionEngine)
        m_pInspectionEngine->abort();
    if (m_pHandlerEngine)
        m_pHandlerEngine->abort();

    // 하드웨어 초기화
    if (m_pHardware)
        m_pHardware->reset();
}

void CMainFrame::OnSeqModeAuto()
{
    m_commandToolBar.setRunMode(RunMode::Auto);
    if (m_pInspectionEngine)
        m_pInspectionEngine->setDebugMode(false);
    if (m_pHandlerEngine)
        m_pHandlerEngine->setDebugMode(false);
}

void CMainFrame::OnSeqModeManual()
{
    m_commandToolBar.setRunMode(RunMode::Manual);
}

void CMainFrame::OnSeqModeDebug()
{
    m_commandToolBar.setRunMode(RunMode::Debug);
    if (m_pInspectionEngine)
        m_pInspectionEngine->setDebugMode(true);
    if (m_pHandlerEngine)
        m_pHandlerEngine->setDebugMode(true);
}

// ============================================================================
// UI 갱신 핸들러
// ============================================================================

void CMainFrame::OnUpdateSeqRun(CCmdUI* pCmdUI)
{
    bool canRun = m_sequencesInitialized &&
        m_pInspectionEngine->getStatus() != EngineStatus::Running;
    pCmdUI->Enable(canRun ? TRUE : FALSE);
}

void CMainFrame::OnUpdateSeqPause(CCmdUI* pCmdUI)
{
    bool canPause = m_pInspectionEngine &&
        m_pInspectionEngine->getStatus() == EngineStatus::Running;
    pCmdUI->Enable(canPause ? TRUE : FALSE);
}

void CMainFrame::OnUpdateSeqStop(CCmdUI* pCmdUI)
{
    bool canStop = m_pInspectionEngine &&
        (m_pInspectionEngine->getStatus() == EngineStatus::Running ||
         m_pInspectionEngine->getStatus() == EngineStatus::Paused);
    pCmdUI->Enable(canStop ? TRUE : FALSE);
}

LRESULT CMainFrame::OnHardwareUpdated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // SimulatedHardware 값 변경 통지 -> 미믹 다이어그램 갱신
    if (m_mimicView.GetSafeHwnd() != nullptr)
    {
        m_mimicView.Invalidate(FALSE);
    }
    return 0;
}

void CMainFrame::updateStatusBar()
{
    if (m_statusBar.GetSafeHwnd() == nullptr)
        return;

    // 모드 표시
    CString strMode;
    switch (m_commandToolBar.getCurrentMode())
    {
    case RunMode::Auto:   strMode = _T("Auto"); break;
    case RunMode::Manual: strMode = _T("Manual"); break;
    case RunMode::Debug:  strMode = _T("Debug"); break;
    }
    m_statusBar.SetPaneText(1, strMode);

    // Inspection 상태
    if (m_pInspectionEngine)
    {
        CString strInspect;
        strInspect.Format(_T("Insp: %s"),
            CString(m_pInspectionEngine->getCurrentStateName().c_str()));
        m_statusBar.SetPaneText(2, strInspect);
    }

    // Handler 상태
    if (m_pHandlerEngine)
    {
        CString strHandler;
        strHandler.Format(_T("Hdlr: %s"),
            CString(m_pHandlerEngine->getCurrentStateName().c_str()));
        m_statusBar.SetPaneText(3, strHandler);
    }

    // 스레드 수
    m_statusBar.SetPaneText(4, _T("Threads: 2"));

    // 에러 수
    m_statusBar.SetPaneText(5, _T("Errors: 0"));

    // 인터락 현황
    if (m_pInspectionEngine && m_pHandlerEngine)
    {
        size_t total = m_pInspectionEngine->getInterlockManager().getInterlockCount()
            + m_pHandlerEngine->getInterlockManager().getInterlockCount();
        CString strIL;
        strIL.Format(_T("IL: %zu OK"), total);
        m_statusBar.SetPaneText(6, strIL);
    }
}

} // namespace YggdraSeq
