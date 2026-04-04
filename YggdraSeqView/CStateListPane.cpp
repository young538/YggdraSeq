/**
 * @file CStateListPane.cpp
 * @brief 상태 목록 도킹 패널 구현
 */

#include "pch.h"
#include "CStateListPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CStateListPane, CWnd)

BEGIN_MESSAGE_MAP(CStateListPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
END_MESSAGE_MAP()

CStateListPane::CStateListPane()
{
}

CStateListPane::~CStateListPane()
{
}

int CStateListPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // CListCtrl 생성 (Report 스타일)
    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_STATE_LIST))
    {
        TRACE0("Failed to create State List CListCtrl\n");
        return -1;
    }

    // 풀 행 선택 + 그리드 라인 스타일
    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    initializeListColumns();

    return 0;
}

void CStateListPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    // CListCtrl을 패널 영역에 맞게 리사이즈
    if (m_listCtrl.GetSafeHwnd() != nullptr)
    {
        m_listCtrl.MoveWindow(0, 0, cx, cy);
    }
}

void CStateListPane::OnPaint()
{
    CPaintDC dc(this);

    // CListCtrl이 있으면 그리기를 CListCtrl에 위임
    CRect rect;
    GetClientRect(rect);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("State List Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CStateListPane::initializeListColumns()
{
    m_listCtrl.InsertColumn(0, _T("Thread ID"), LVCFMT_LEFT, 80);
    m_listCtrl.InsertColumn(1, _T("Current State"), LVCFMT_LEFT, 120);
    m_listCtrl.InsertColumn(2, _T("Engine Status"), LVCFMT_LEFT, 100);
    m_listCtrl.InsertColumn(3, _T("Entry Time"), LVCFMT_LEFT, 100);
    m_listCtrl.InsertColumn(4, _T("Elapsed"), LVCFMT_LEFT, 80);
}

int CStateListPane::findOrCreateRow(uint32_t threadId)
{
    auto it = m_threadInfoMap.find(threadId);
    if (it != m_threadInfoMap.end() && it->second.listIndex >= 0)
    {
        return it->second.listIndex;
    }

    // 새 행 추가
    CString strThreadId;
    strThreadId.Format(_T("T-%u"), threadId);
    int nRow = m_listCtrl.GetItemCount();
    m_listCtrl.InsertItem(nRow, strThreadId);

    m_threadInfoMap[threadId].listIndex = nRow;
    return nRow;
}

void CStateListPane::updateStateChange(
    uint32_t threadId, const std::string& /*fromState*/, const std::string& toState,
    const std::chrono::system_clock::time_point& timestamp)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    int nRow = findOrCreateRow(threadId);
    auto& info = m_threadInfoMap[threadId];
    info.currentState = toState;
    info.entryTime = timestamp;

    // 현재 상태 이름 갱신
    m_listCtrl.SetItemText(nRow, 1, CString(toState.c_str()));

    // 진입 시간 포맷
    auto timeT = std::chrono::system_clock::to_time_t(timestamp);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    CString strTime;
    strTime.Format(_T("%02d:%02d:%02d"), tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    m_listCtrl.SetItemText(nRow, 3, strTime);

    // 경과 시간 초기화
    m_listCtrl.SetItemText(nRow, 4, _T("0.0s"));
}

void CStateListPane::updateEngineStatus(uint32_t threadId, EngineStatus newStatus)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    int nRow = findOrCreateRow(threadId);
    m_threadInfoMap[threadId].engineStatus = newStatus;

    CString strStatus(engineStatusToString(newStatus));
    m_listCtrl.SetItemText(nRow, 2, strStatus);
}

} // namespace YggdraSeq
