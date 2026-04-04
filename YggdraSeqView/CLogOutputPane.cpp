/**
 * @file CLogOutputPane.cpp
 * @brief 실시간 로그 출력 도킹 패널 구현
 *
 * 로그 레벨별 색상:
 *   - TRACE/DEBUG: 회색 텍스트
 *   - INFO: 흰색 텍스트
 *   - WARN: 노란색 텍스트
 *   - ERROR: 빨간색 텍스트
 *   - CRITICAL: 빨간 배경 + 흰 텍스트
 */

#include "pch.h"
#include "CLogOutputPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CLogOutputPane, CDockablePane)

BEGIN_MESSAGE_MAP(CLogOutputPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOG_LIST, &CLogOutputPane::OnCustomDraw)
END_MESSAGE_MAP()

CLogOutputPane::CLogOutputPane()
    : m_minLevelFilter(LogLevel::Trace)
    , m_threadFilter(0)
    , m_autoScroll(true)
{
}

CLogOutputPane::~CLogOutputPane()
{
}

int CLogOutputPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_OWNERDATA;  // 가상 리스트 (대용량 로그 대응)

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_LOG_LIST))
    {
        TRACE0("Failed to create Log CListCtrl\n");
        return -1;
    }

    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    initializeListColumns();

    return 0;
}

void CLogOutputPane::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.MoveWindow(0, 0, cx, cy);
}

void CLogOutputPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(rect);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("Log Output Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CLogOutputPane::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
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
        if (nItem >= 0 && nItem < static_cast<int>(m_allEntries.size()))
        {
            const auto& entry = m_allEntries[nItem];
            // 다크 테마 배경
            pLVCD->clrTextBk = RGB(30, 30, 30);

            switch (entry.level)
            {
            case LogLevel::Trace:
            case LogLevel::Debug:
                pLVCD->clrText = RGB(128, 128, 128);   // 회색
                break;
            case LogLevel::Info:
                pLVCD->clrText = RGB(220, 220, 220);   // 밝은 회색
                break;
            case LogLevel::Warn:
                pLVCD->clrText = RGB(255, 220, 50);    // 노랑
                break;
            case LogLevel::Error:
                pLVCD->clrText = RGB(255, 80, 80);     // 빨강
                break;
            case LogLevel::Critical:
                pLVCD->clrTextBk = RGB(200, 30, 30);   // 빨간 배경
                pLVCD->clrText = RGB(255, 255, 255);   // 흰 텍스트
                break;
            }
        }
        *pResult = CDRF_DODEFAULT;
        break;
    }
    }
}

void CLogOutputPane::initializeListColumns()
{
    m_listCtrl.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 85);
    m_listCtrl.InsertColumn(1, _T("Thread"), LVCFMT_CENTER, 55);
    m_listCtrl.InsertColumn(2, _T("Level"), LVCFMT_CENTER, 60);
    m_listCtrl.InsertColumn(3, _T("Source"), LVCFMT_LEFT, 90);
    m_listCtrl.InsertColumn(4, _T("Message"), LVCFMT_LEFT, 400);
}

bool CLogOutputPane::passesFilter(const LogEntry& entry) const
{
    // 레벨 필터
    if (static_cast<int>(entry.level) < static_cast<int>(m_minLevelFilter))
        return false;

    // 스레드 필터
    if (m_threadFilter != 0 && entry.threadId != m_threadFilter)
        return false;

    // 키워드 필터
    if (!m_keywordFilter.empty())
    {
        if (entry.message.find(m_keywordFilter) == std::string::npos &&
            entry.source.find(m_keywordFilter) == std::string::npos)
            return false;
    }

    return true;
}

void CLogOutputPane::addLogEntry(
    uint32_t threadId, LogLevel level, const std::string& source,
    const std::string& message, const std::chrono::system_clock::time_point& timestamp)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    LogEntry entry;
    entry.threadId = threadId;
    entry.level = level;
    entry.source = source;
    entry.message = message;
    entry.timestamp = timestamp;

    // 최대 항목 수 초과 시 오래된 항목 삭제
    if (m_allEntries.size() >= MAX_LOG_ENTRIES)
    {
        m_allEntries.pop_front();
        // 리스트도 첫 행 삭제
        if (m_listCtrl.GetItemCount() > 0)
            m_listCtrl.DeleteItem(0);
    }

    m_allEntries.push_back(entry);

    // 필터를 통과하는 경우에만 리스트에 추가
    if (passesFilter(entry))
    {
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
        m_listCtrl.SetItemText(nRow, 2, CString(logLevelToString(level)));
        m_listCtrl.SetItemText(nRow, 3, CString(source.c_str()));
        m_listCtrl.SetItemText(nRow, 4, CString(message.c_str()));

        // 자동 스크롤
        if (m_autoScroll)
        {
            m_listCtrl.EnsureVisible(nRow, FALSE);
        }
    }
}

void CLogOutputPane::clearLog()
{
    m_allEntries.clear();
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.DeleteAllItems();
}

void CLogOutputPane::setLevelFilter(LogLevel minLevel)
{
    m_minLevelFilter = minLevel;
    rebuildFilteredList();
}

void CLogOutputPane::setThreadFilter(uint32_t threadId)
{
    m_threadFilter = threadId;
    rebuildFilteredList();
}

void CLogOutputPane::setKeywordFilter(const std::string& keyword)
{
    m_keywordFilter = keyword;
    rebuildFilteredList();
}

void CLogOutputPane::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
}

void CLogOutputPane::rebuildFilteredList()
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    m_listCtrl.SetRedraw(FALSE);
    m_listCtrl.DeleteAllItems();

    int nRow = 0;
    for (const auto& entry : m_allEntries)
    {
        if (!passesFilter(entry))
            continue;

        auto timeT = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);
        CString strTime;
        strTime.Format(_T("%02d:%02d:%02d.%03d"), tmBuf.tm_hour, tmBuf.tm_min,
            tmBuf.tm_sec, static_cast<int>(ms.count()));

        m_listCtrl.InsertItem(nRow, strTime);

        CString strThread;
        strThread.Format(_T("T-%u"), entry.threadId);
        m_listCtrl.SetItemText(nRow, 1, strThread);
        m_listCtrl.SetItemText(nRow, 2, CString(logLevelToString(entry.level)));
        m_listCtrl.SetItemText(nRow, 3, CString(entry.source.c_str()));
        m_listCtrl.SetItemText(nRow, 4, CString(entry.message.c_str()));

        nRow++;
    }

    m_listCtrl.SetRedraw(TRUE);

    if (m_autoScroll && nRow > 0)
        m_listCtrl.EnsureVisible(nRow - 1, FALSE);
}

bool CLogOutputPane::exportToFile(const std::string& filePath) const
{
    std::ofstream ofs(filePath);
    if (!ofs.is_open())
        return false;

    ofs << "Time\tThread\tLevel\tSource\tMessage\n";

    for (const auto& entry : m_allEntries)
    {
        auto timeT = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);

        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d.%03d",
            tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec, static_cast<int>(ms.count()));

        ofs << timeBuf << "\t"
            << "T-" << entry.threadId << "\t"
            << logLevelToString(entry.level) << "\t"
            << entry.source << "\t"
            << entry.message << "\n";
    }

    return true;
}

} // namespace YggdraSeq
