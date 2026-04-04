/**
 * @file CCommandToolBar.cpp
 * @brief 상단 명령 툴바 구현
 *
 * 프로그래밍 방식으로 버튼을 추가합니다 (리소스 비트맵 없이 동작).
 */

#include "pch.h"
#include "CCommandToolBar.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CCommandToolBar, CMFCToolBar)

BEGIN_MESSAGE_MAP(CCommandToolBar, CMFCToolBar)
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
    if (!CMFCToolBar::Create(pParentWnd, AFX_DEFAULT_TOOLBAR_STYLE, IDR_MAIN_TOOLBAR))
    {
        TRACE0("Failed to create Command ToolBar\n");
        return FALSE;
    }

    // 버튼 크기 설정
    SetPaneStyle(GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS);

    // 버튼 추가 (프로그래밍 방식, 아이콘 없이 텍스트 버튼)
    CMFCToolBarButton btnRun(ID_SEQ_RUN, -1, _T("Run"));
    InsertButton(btnRun);

    CMFCToolBarButton btnPause(ID_SEQ_PAUSE, -1, _T("Pause"));
    InsertButton(btnPause);

    CMFCToolBarButton btnStop(ID_SEQ_STOP, -1, _T("Stop"));
    InsertButton(btnStop);

    // 구분선
    InsertSeparator();

    CMFCToolBarButton btnEmo(ID_SEQ_EMO, -1, _T("EMO"));
    InsertButton(btnEmo);

    InsertSeparator();

    // 모드 선택 버튼
    CMFCToolBarButton btnAuto(ID_SEQ_MODE_AUTO, -1, _T("Auto"));
    InsertButton(btnAuto);

    CMFCToolBarButton btnManual(ID_SEQ_MODE_MANUAL, -1, _T("Manual"));
    InsertButton(btnManual);

    CMFCToolBarButton btnDebug(ID_SEQ_MODE_DEBUG, -1, _T("Debug"));
    InsertButton(btnDebug);

    return TRUE;
}

void CCommandToolBar::updateLedIndicator(EngineStatus status)
{
    m_currentStatus = status;
    // LED 색상은 MainFrm의 상태 바에서 갱신
    Invalidate();
}

} // namespace YggdraSeq
