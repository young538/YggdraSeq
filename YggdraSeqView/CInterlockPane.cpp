/**
 * @file CInterlockPane.cpp
 * @brief 인터락 상태 도킹 패널 구현
 *
 * 인터락 체크 결과를 수신하여 신호등 스타일로 색상 표시합니다.
 * CListCtrl의 NM_CUSTOMDRAW를 활용하여 행 배경색을 제어합니다.
 */

#include "pch.h"
#include "CInterlockPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CInterlockPane, CDockablePane)

BEGIN_MESSAGE_MAP(CInterlockPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_INTERLOCK_LIST, &CInterlockPane::OnCustomDraw)
END_MESSAGE_MAP()

CInterlockPane::CInterlockPane()
{
}

CInterlockPane::~CInterlockPane()
{
}

int CInterlockPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_INTERLOCK_LIST))
    {
        TRACE0("Failed to create Interlock CListCtrl\n");
        return -1;
    }

    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    initializeListColumns();

    return 0;
}

void CInterlockPane::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.MoveWindow(0, 0, cx, cy);
}

void CInterlockPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(rect);

    if (m_listCtrl.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("Interlock Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CInterlockPane::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
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

        // 해당 행의 인터락 이름으로 상태 조회
        CString strName = m_listCtrl.GetItemText(nItem, 0);
        std::string name{CT2A(strName)};

        auto it = m_interlockInfoMap.find(name);
        if (it != m_interlockInfoMap.end())
        {
            if (!it->second.passed)
            {
                // 위반 상태: 심각도에 따라 색상 결정
                if (it->second.severity == InterlockSeverity::Warning)
                {
                    // 노랑 배경 (경고)
                    pLVCD->clrTextBk = RGB(255, 255, 100);
                    pLVCD->clrText = RGB(0, 0, 0);
                }
                else
                {
                    // 빨강 배경 (Critical/EMO)
                    pLVCD->clrTextBk = RGB(255, 80, 80);
                    pLVCD->clrText = RGB(255, 255, 255);
                }
            }
            else
            {
                // 정상: 초록 배경
                pLVCD->clrTextBk = RGB(80, 200, 80);
                pLVCD->clrText = RGB(0, 0, 0);
            }
        }

        *pResult = CDRF_DODEFAULT;
        break;
    }
    }
}

void CInterlockPane::initializeListColumns()
{
    m_listCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 140);
    m_listCtrl.InsertColumn(1, _T("Status"), LVCFMT_CENTER, 70);
    m_listCtrl.InsertColumn(2, _T("Severity"), LVCFMT_CENTER, 70);
    m_listCtrl.InsertColumn(3, _T("Current"), LVCFMT_RIGHT, 80);
    m_listCtrl.InsertColumn(4, _T("Threshold"), LVCFMT_RIGHT, 80);
    m_listCtrl.InsertColumn(5, _T("Thread"), LVCFMT_CENTER, 60);
    m_listCtrl.InsertColumn(6, _T("Last Check"), LVCFMT_CENTER, 80);
}

int CInterlockPane::findOrCreateRow(const std::string& interlockName)
{
    auto it = m_interlockInfoMap.find(interlockName);
    if (it != m_interlockInfoMap.end() && it->second.listIndex >= 0)
        return it->second.listIndex;

    int nRow = m_listCtrl.GetItemCount();
    m_listCtrl.InsertItem(nRow, CString(interlockName.c_str()));
    m_interlockInfoMap[interlockName].listIndex = nRow;
    return nRow;
}

void CInterlockPane::updateInterlockStatus(
    uint32_t threadId, const std::string& interlockName,
    const std::string& currentValue, bool passed,
    const std::chrono::system_clock::time_point& timestamp)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    int nRow = findOrCreateRow(interlockName);
    auto& info = m_interlockInfoMap[interlockName];
    info.passed = passed;
    info.currentValue = currentValue;

    m_listCtrl.SetItemText(nRow, 1, passed ? _T("OK") : _T("FAIL"));
    m_listCtrl.SetItemText(nRow, 3, CString(currentValue.c_str()));

    CString strThread;
    strThread.Format(_T("T-%u"), threadId);
    m_listCtrl.SetItemText(nRow, 5, strThread);

    // 시간 포맷
    auto timeT = std::chrono::system_clock::to_time_t(timestamp);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    CString strTime;
    strTime.Format(_T("%02d:%02d:%02d"), tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    m_listCtrl.SetItemText(nRow, 6, strTime);

    // 행 다시 그리기 (색상 갱신)
    m_listCtrl.RedrawItems(nRow, nRow);
}

void CInterlockPane::updateInterlockViolation(
    uint32_t threadId, const std::string& interlockName,
    InterlockSeverity severity, const std::string& currentValue,
    const std::string& thresholdValue,
    const std::chrono::system_clock::time_point& timestamp)
{
    if (m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    int nRow = findOrCreateRow(interlockName);
    auto& info = m_interlockInfoMap[interlockName];
    info.passed = false;
    info.severity = severity;
    info.currentValue = currentValue;
    info.thresholdValue = thresholdValue;

    m_listCtrl.SetItemText(nRow, 1, _T("FAIL"));
    m_listCtrl.SetItemText(nRow, 2, CString(interlockSeverityToString(severity)));
    m_listCtrl.SetItemText(nRow, 3, CString(currentValue.c_str()));
    m_listCtrl.SetItemText(nRow, 4, CString(thresholdValue.c_str()));

    CString strThread;
    strThread.Format(_T("T-%u"), threadId);
    m_listCtrl.SetItemText(nRow, 5, strThread);

    auto timeT = std::chrono::system_clock::to_time_t(timestamp);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    CString strTime;
    strTime.Format(_T("%02d:%02d:%02d"), tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    m_listCtrl.SetItemText(nRow, 6, strTime);

    m_listCtrl.RedrawItems(nRow, nRow);
}

} // namespace YggdraSeq
