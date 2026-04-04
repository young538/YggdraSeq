/**
 * @file CTransitionLogPane.cpp
 * @brief 전이 이력 도킹 패널 구현
 *
 * 전이 성공은 기본 배경, 실패는 연한 빨간 배경으로 표시합니다.
 */

#include "pch.h"
#include "CTransitionLogPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CTransitionLogPane, CWnd)

BEGIN_MESSAGE_MAP(CTransitionLogPane, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_TRANSITION_LIST, &CTransitionLogPane::OnCustomDraw)
END_MESSAGE_MAP()

CTransitionLogPane::CTransitionLogPane()
    : m_failedOnlyFilter(false)
    , m_threadFilter(0)
{
}

CTransitionLogPane::~CTransitionLogPane()
{
}

int CTransitionLogPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_TRANSITION_LIST))
    {
        TRACE0("Failed to create Transition Log CListCtrl\n");
        return -1;
    }

    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    initializeListColumns();

    return 0;
}

void CTransitionLogPane::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.MoveWindow(0, 0, cx, cy);
}

void CTransitionLogPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(rect);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("Transition Log Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CTransitionLogPane::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
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
        pLVCD->clrText = RGB(200, 200, 200);

        // 결과 컬럼에서 성공/실패 판별
        CString strResult = m_listCtrl.GetItemText(nItem, 4);
        if (strResult == _T("FAIL"))
        {
            pLVCD->clrTextBk = RGB(80, 30, 30);    // 연한 빨간 배경
            pLVCD->clrText = RGB(255, 150, 150);    // 밝은 빨간 텍스트
        }
        else if (strResult == _T("OK"))
        {
            pLVCD->clrText = RGB(120, 220, 120);    // 초록 텍스트
        }

        *pResult = CDRF_DODEFAULT;
        break;
    }
    }
}

void CTransitionLogPane::initializeListColumns()
{
    m_listCtrl.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 85);
    m_listCtrl.InsertColumn(1, _T("Thread"), LVCFMT_CENTER, 55);
    m_listCtrl.InsertColumn(2, _T("From"), LVCFMT_LEFT, 80);
    m_listCtrl.InsertColumn(3, _T("To"), LVCFMT_LEFT, 80);
    m_listCtrl.InsertColumn(4, _T("Result"), LVCFMT_CENTER, 50);
    m_listCtrl.InsertColumn(5, _T("Guard"), LVCFMT_CENTER, 50);
    m_listCtrl.InsertColumn(6, _T("Interlocks"), LVCFMT_LEFT, 200);
}

bool CTransitionLogPane::passesFilter(const TransitionRecord& record) const
{
    if (m_failedOnlyFilter && record.overallSuccess)
        return false;

    if (m_threadFilter != 0 && record.threadId != m_threadFilter)
        return false;

    return true;
}

void CTransitionLogPane::addTransitionRecord(
    uint32_t threadId, const std::string& fromState, const std::string& toState,
    bool guardResult, const std::vector<InterlockCheckResult>& interlockResults,
    const std::chrono::system_clock::time_point& timestamp)
{
    TransitionRecord record;
    record.threadId = threadId;
    record.fromState = fromState;
    record.toState = toState;
    record.guardResult = guardResult;
    record.interlockResults = interlockResults;
    record.timestamp = timestamp;

    // 전체 성공 여부: guard + 모든 인터락 통과
    record.overallSuccess = guardResult;
    for (const auto& ir : interlockResults)
    {
        if (!ir.passed)
        {
            record.overallSuccess = false;
            break;
        }
    }

    // 최대 기록 수 초과 시 오래된 기록 삭제
    if (m_allRecords.size() >= MAX_RECORDS)
    {
        m_allRecords.pop_front();
        if (m_listCtrl.GetSafeHwnd() != nullptr && m_listCtrl.GetItemCount() > 0)
            m_listCtrl.DeleteItem(0);
    }

    m_allRecords.push_back(record);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    // 필터 통과 시에만 리스트에 추가
    if (!passesFilter(record))
        return;

    int nRow = m_listCtrl.GetItemCount();

    // 시간 포맷
    auto timeT = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    CString strTime;
    strTime.Format(_T("%02d:%02d:%02d.%03d"), tmBuf.tm_hour, tmBuf.tm_min,
        tmBuf.tm_sec, static_cast<int>(ms.count()));

    m_listCtrl.InsertItem(nRow, strTime);

    CString strThread;
    strThread.Format(_T("T-%u"), threadId);
    m_listCtrl.SetItemText(nRow, 1, strThread);
    m_listCtrl.SetItemText(nRow, 2, CString(fromState.c_str()));
    m_listCtrl.SetItemText(nRow, 3, CString(toState.c_str()));
    m_listCtrl.SetItemText(nRow, 4, record.overallSuccess ? _T("OK") : _T("FAIL"));
    m_listCtrl.SetItemText(nRow, 5, guardResult ? _T("TRUE") : _T("FALSE"));

    // 인터락 결과 조합
    CString strInterlocks;
    for (const auto& ir : interlockResults)
    {
        if (!strInterlocks.IsEmpty())
            strInterlocks += _T(", ");
        strInterlocks += CString(ir.interlockName.c_str());
        strInterlocks += ir.passed ? _T("=OK") : _T("=FAIL");
    }
    m_listCtrl.SetItemText(nRow, 6, strInterlocks);

    m_listCtrl.EnsureVisible(nRow, FALSE);
}

void CTransitionLogPane::clearRecords()
{
    m_allRecords.clear();
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.DeleteAllItems();
}

void CTransitionLogPane::setFailedOnlyFilter(bool failedOnly)
{
    m_failedOnlyFilter = failedOnly;
    rebuildFilteredList();
}

void CTransitionLogPane::setThreadFilter(uint32_t threadId)
{
    m_threadFilter = threadId;
    rebuildFilteredList();
}

void CTransitionLogPane::rebuildFilteredList()
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    m_listCtrl.SetRedraw(FALSE);
    m_listCtrl.DeleteAllItems();

    for (const auto& record : m_allRecords)
    {
        if (!passesFilter(record))
            continue;

        int nRow = m_listCtrl.GetItemCount();

        auto timeT = std::chrono::system_clock::to_time_t(record.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.timestamp.time_since_epoch()) % 1000;
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);
        CString strTime;
        strTime.Format(_T("%02d:%02d:%02d.%03d"), tmBuf.tm_hour, tmBuf.tm_min,
            tmBuf.tm_sec, static_cast<int>(ms.count()));

        m_listCtrl.InsertItem(nRow, strTime);

        CString strThread;
        strThread.Format(_T("T-%u"), record.threadId);
        m_listCtrl.SetItemText(nRow, 1, strThread);
        m_listCtrl.SetItemText(nRow, 2, CString(record.fromState.c_str()));
        m_listCtrl.SetItemText(nRow, 3, CString(record.toState.c_str()));
        m_listCtrl.SetItemText(nRow, 4, record.overallSuccess ? _T("OK") : _T("FAIL"));
        m_listCtrl.SetItemText(nRow, 5, record.guardResult ? _T("TRUE") : _T("FALSE"));

        CString strInterlocks;
        for (const auto& ir : record.interlockResults)
        {
            if (!strInterlocks.IsEmpty())
                strInterlocks += _T(", ");
            strInterlocks += CString(ir.interlockName.c_str());
            strInterlocks += ir.passed ? _T("=OK") : _T("=FAIL");
        }
        m_listCtrl.SetItemText(nRow, 6, strInterlocks);
    }

    m_listCtrl.SetRedraw(TRUE);
}

bool CTransitionLogPane::exportToCsv(const std::string& filePath) const
{
    std::ofstream ofs(filePath);
    if (!ofs.is_open())
        return false;

    ofs << "Time,Thread,From,To,Result,Guard,Interlocks\n";

    for (const auto& record : m_allRecords)
    {
        auto timeT = std::chrono::system_clock::to_time_t(record.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.timestamp.time_since_epoch()) % 1000;
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);

        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d.%03d",
            tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec, static_cast<int>(ms.count()));

        ofs << timeBuf << ","
            << "T-" << record.threadId << ","
            << record.fromState << ","
            << record.toState << ","
            << (record.overallSuccess ? "OK" : "FAIL") << ","
            << (record.guardResult ? "TRUE" : "FALSE") << ",\"";

        bool first = true;
        for (const auto& ir : record.interlockResults)
        {
            if (!first) ofs << ", ";
            ofs << ir.interlockName << "=" << (ir.passed ? "OK" : "FAIL");
            first = false;
        }
        ofs << "\"\n";
    }

    return true;
}

} // namespace YggdraSeq
