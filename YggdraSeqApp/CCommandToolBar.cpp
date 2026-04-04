/**
 * @file CCommandToolBar.cpp
 * @brief 상단 명령 버튼 바 구현
 *
 * CButton을 사용하여 시퀀스 제어 버튼과 모드 버튼을 배치합니다.
 * 각 버튼 클릭은 부모 프레임에 WM_COMMAND로 전달됩니다.
 */

#include "pch.h"
#include "CCommandToolBar.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CCommandToolBar, CWnd)

BEGIN_MESSAGE_MAP(CCommandToolBar, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    // 버튼 클릭 -> 부모 프레임에 WM_COMMAND 전달
    ON_COMMAND_RANGE(ID_SEQ_RUN, ID_SEQ_MODE_DEBUG, &CCommandToolBar::OnButtonClicked)
END_MESSAGE_MAP()

CCommandToolBar::CCommandToolBar()
    : m_currentMode(RunMode::Auto)
    , m_currentStatus(EngineStatus::Idle)
{
}

CCommandToolBar::~CCommandToolBar()
{
}

BOOL CCommandToolBar::CreateToolBar(CWnd* pParentWnd)
{
    // CWnd로 생성 (메인 뷰 상단에 고정 배치)
    CRect rect(0, 0, 800, COMMAND_BAR_HEIGHT);
    return Create(AfxRegisterWndClass(0), _T("CommandBar"),
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        rect, pParentWnd, 0);
}

int CCommandToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;

    // 시퀀스 제어 버튼 생성
    m_btnRun.Create(_T("Run"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_RUN);
    m_btnPause.Create(_T("Pause"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_PAUSE);
    m_btnStop.Create(_T("Stop"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_STOP);
    m_btnEmo.Create(_T("EMO"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_EMO);

    // 모드 버튼 생성
    m_btnAuto.Create(_T("Auto"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_MODE_AUTO);
    m_btnManual.Create(_T("Manual"), dwBtnStyle, CRect(0, 0, 70, 26), this, ID_SEQ_MODE_MANUAL);
    m_btnDebug.Create(_T("Debug"), dwBtnStyle, CRect(0, 0, 60, 26), this, ID_SEQ_MODE_DEBUG);

    layoutButtons();
    return 0;
}

void CCommandToolBar::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    layoutButtons();
}

BOOL CCommandToolBar::OnEraseBkgnd(CDC* pDC)
{
    CRect rect;
    GetClientRect(rect);
    pDC->FillSolidRect(rect, RGB(45, 45, 48));
    return TRUE;
}

void CCommandToolBar::layoutButtons()
{
    if (m_btnRun.GetSafeHwnd() == nullptr)
        return;

    const int margin = 4;
    const int btnH = 24;
    const int btnW = 60;
    const int btnWide = 70;
    const int gap = 2;
    const int sepGap = 12;   // 구분 간격
    int y = (COMMAND_BAR_HEIGHT - btnH) / 2;
    int x = margin;

    // [Run] [Pause] [Stop]
    m_btnRun.MoveWindow(x, y, btnW, btnH); x += btnW + gap;
    m_btnPause.MoveWindow(x, y, btnW, btnH); x += btnW + gap;
    m_btnStop.MoveWindow(x, y, btnW, btnH); x += btnW + sepGap;

    // [EMO]
    m_btnEmo.MoveWindow(x, y, btnW, btnH); x += btnW + sepGap;

    // [Auto] [Manual] [Debug]
    m_btnAuto.MoveWindow(x, y, btnW, btnH); x += btnW + gap;
    m_btnManual.MoveWindow(x, y, btnWide, btnH); x += btnWide + gap;
    m_btnDebug.MoveWindow(x, y, btnW, btnH);
}

void CCommandToolBar::OnButtonClicked(UINT nID)
{
    // 버튼 클릭을 부모 프레임에 전달
    GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(nID, BN_CLICKED), 0);
}

void CCommandToolBar::updateLedIndicator(EngineStatus status)
{
    m_currentStatus = status;
    Invalidate();
}

} // namespace YggdraSeq
