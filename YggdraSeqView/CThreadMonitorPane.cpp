/**
 * @file CThreadMonitorPane.cpp
 * @brief 스레드 모니터 도킹 패널 구현
 *
 * 스레드 상태별 색상:
 *   - Running: 초록 텍스트
 *   - Waiting: 노랑 텍스트
 *   - Blocked: 주황 텍스트
 *   - Idle: 회색 텍스트
 *   - Terminated: 어두운 회색 텍스트
 *   - 데드락 의심: 빨간 배경
 */

#include "pch.h"
#include "CThreadMonitorPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CThreadMonitorPane, CWnd)

BEGIN_MESSAGE_MAP(CThreadMonitorPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_THREAD_LIST, &CThreadMonitorPane::OnCustomDraw)
END_MESSAGE_MAP()

CThreadMonitorPane::CThreadMonitorPane()
{
}

CThreadMonitorPane::~CThreadMonitorPane()
{
}

int CThreadMonitorPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_THREAD_LIST))
    {
        TRACE0("Failed to create Thread Monitor CListCtrl\n");
        return -1;
    }

    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    initializeListColumns();

    return 0;
}

void CThreadMonitorPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.MoveWindow(0, 0, cx, cy);
}

void CThreadMonitorPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(rect);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("Thread Monitor Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CThreadMonitorPane::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_ITEMPREPAINT:
    {
        int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
        pLVCD->clrTextBk = RGB(30, 30, 30);

        // 스레드 ID를 행 텍스트에서 가져와서 상태 조회
        CString strId = m_listCtrl.GetItemText(nItem, 0);
        uint32_t tid = 0;
        _stscanf_s(strId, _T("T-%u"), &tid);

        auto it = m_threadMap.find(tid);
        if (it != m_threadMap.end())
        {
            // 데드락 의심 시 빨간 배경
            if (it->second.deadlockSuspect)
            {
                pLVCD->clrTextBk = RGB(180, 30, 30);
                pLVCD->clrText = RGB(255, 255, 255);
            }
            else
            {
                switch (it->second.state)
                {
                case ThreadState::Running:
                    pLVCD->clrText = RGB(80, 220, 80);    // 초록
                    break;
                case ThreadState::Waiting:
                    pLVCD->clrText = RGB(255, 220, 50);   // 노랑
                    break;
                case ThreadState::Blocked:
                    pLVCD->clrText = RGB(255, 160, 50);   // 주황
                    break;
                case ThreadState::Idle:
                    pLVCD->clrText = RGB(128, 128, 128);  // 회색
                    break;
                case ThreadState::Terminated:
                    pLVCD->clrText = RGB(80, 80, 80);     // 어두운 회색
                    break;
                }
            }
        }

        *pResult = CDRF_DODEFAULT;
        break;
    }
    }
}

void CThreadMonitorPane::initializeListColumns()
{
    m_listCtrl.InsertColumn(0, _T("Thread ID"), LVCFMT_LEFT, 70);
    m_listCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 100);
    m_listCtrl.InsertColumn(2, _T("State"), LVCFMT_CENTER, 80);
    m_listCtrl.InsertColumn(3, _T("Lock Info"), LVCFMT_LEFT, 150);
    m_listCtrl.InsertColumn(4, _T("Last Activity"), LVCFMT_CENTER, 90);
}

int CThreadMonitorPane::findOrCreateRow(uint32_t threadId)
{
    auto it = m_threadMap.find(threadId);
    if (it != m_threadMap.end() && it->second.listIndex >= 0)
        return it->second.listIndex;

    CString strId;
    strId.Format(_T("T-%u"), threadId);
    int nRow = m_listCtrl.GetItemCount();
    m_listCtrl.InsertItem(nRow, strId);
    m_threadMap[threadId].listIndex = nRow;
    return nRow;
}

bool CThreadMonitorPane::detectDeadlock() const
{
    // 간단한 2-스레드 순환 대기 감지
    // Blocked 상태인 스레드가 2개 이상이면 데드락 의심
    int blockedCount = 0;
    for (const auto& pair : m_threadMap)
    {
        if (pair.second.state == ThreadState::Blocked)
            blockedCount++;
    }
    return blockedCount >= 2;
}

void CThreadMonitorPane::updateThreadState(
    uint32_t threadId, const std::string& threadName,
    ThreadState newState, const std::string& lockInfo,
    const std::chrono::system_clock::time_point& timestamp)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    int nRow = findOrCreateRow(threadId);
    auto& info = m_threadMap[threadId];
    info.threadName = threadName;
    info.state = newState;
    info.lockInfo = lockInfo;
    info.lastActivity = timestamp;

    m_listCtrl.SetItemText(nRow, 1, CString(threadName.c_str()));
    m_listCtrl.SetItemText(nRow, 2, CString(threadStateToString(newState)));
    m_listCtrl.SetItemText(nRow, 3, CString(lockInfo.c_str()));

    auto timeT = std::chrono::system_clock::to_time_t(timestamp);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    CString strTime;
    strTime.Format(_T("%02d:%02d:%02d"), tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    m_listCtrl.SetItemText(nRow, 4, strTime);

    // 데드락 감지
    bool deadlock = detectDeadlock();
    for (auto& pair : m_threadMap)
    {
        pair.second.deadlockSuspect = (deadlock && pair.second.state == ThreadState::Blocked);
    }

    // 전체 다시 그리기 (데드락 색상 갱신)
    m_listCtrl.Invalidate();
}

} // namespace YggdraSeq
