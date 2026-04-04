/**
 * @file YggdraSeqApp.cpp
 * @brief YggdraSeqApp MFC Application 구현
 *
 * CWinApp::InitInstance()에서 메인 프레임을 생성하고 표시합니다.
 * VS Code 스타일 다크 테마를 적용합니다.
 */

#include "pch.h"
#include "YggdraSeqApp.h"
#include "MainFrm.h"

namespace YggdraSeq
{

BEGIN_MESSAGE_MAP(CYggdraSeqApp, CWinApp)
END_MESSAGE_MAP()

// ============================================================================
// 전역 앱 인스턴스
// ============================================================================

CYggdraSeqApp theApp;

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

CYggdraSeqApp::CYggdraSeqApp()
{
    // 재시작 매니저 지원
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CYggdraSeqApp::~CYggdraSeqApp()
{
}

// ============================================================================
// 애플리케이션 초기화
// ============================================================================

BOOL CYggdraSeqApp::InitInstance()
{
    // MFC 초기화
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    CWinApp::InitInstance();

    // OLE 초기화 (MFC Feature Pack 도킹에 필요)
    if (!AfxOleInit())
    {
        AfxMessageBox(_T("OLE initialization failed"));
        return FALSE;
    }

    AfxEnableControlContainer();

    // 레지스트리 키 설정 (도킹 레이아웃 저장용)
    SetRegistryKey(_T("YggdraSeq"));

    // 다크 테마 비주얼 매니저 (VS Code 스타일)
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
    CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);

    // 메인 프레임 생성
    auto* pMainFrame = new CMainFrame();
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
    {
        delete pMainFrame;
        return FALSE;
    }
    m_pMainWnd = pMainFrame;

    // 메인 프레임 표시
    pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
    pMainFrame->UpdateWindow();

    return TRUE;
}

int CYggdraSeqApp::ExitInstance()
{
    AfxOleTerm(FALSE);
    return CWinApp::ExitInstance();
}

} // namespace YggdraSeq
