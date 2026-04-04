/**
 * @file CSequenceDebugPane.cpp
 * @brief 시퀀스 디버그 도킹 패널 구현
 *
 * 패널 레이아웃 (3분할):
 *   상단: 전이 조건 모니터 (CListCtrl)
 *   중간: 변수 워치 (CListCtrl)
 *   하단: 콜스택 (CListCtrl)
 */

#include "pch.h"
#include "CSequenceDebugPane.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CSequenceDebugPane, CDockablePane)

BEGIN_MESSAGE_MAP(CSequenceDebugPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
END_MESSAGE_MAP()

CSequenceDebugPane::CSequenceDebugPane()
{
}

CSequenceDebugPane::~CSequenceDebugPane()
{
}

int CSequenceDebugPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    initializeControls();
    return 0;
}

void CSequenceDebugPane::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);
    layoutControls(cx, cy);
}

void CSequenceDebugPane::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(rect);

    if (m_transitionList.GetSafeHwnd() == nullptr)
    {
        dc.FillSolidRect(rect, RGB(30, 30, 30));
        dc.SetTextColor(RGB(200, 200, 200));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(_T("Sequence Debug Pane"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void CSequenceDebugPane::initializeControls()
{
    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    // 전이 조건 모니터 리스트
    m_transitionList.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_DEBUG_TRANSITION_LIST);
    m_transitionList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_transitionList.InsertColumn(0, _T("Transition"), LVCFMT_LEFT, 150);
    m_transitionList.InsertColumn(1, _T("Guard"), LVCFMT_CENTER, 60);
    m_transitionList.InsertColumn(2, _T("Interlocks"), LVCFMT_LEFT, 200);

    // 변수 워치 리스트
    m_watchList.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_DEBUG_WATCH_LIST);
    m_watchList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_watchList.InsertColumn(0, _T("Parameter"), LVCFMT_LEFT, 120);
    m_watchList.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 120);
    m_watchList.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 80);

    // 콜스택 리스트
    m_callStackList.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_DEBUG_CALLSTACK_LIST);
    m_callStackList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_callStackList.InsertColumn(0, _T("#"), LVCFMT_CENTER, 30);
    m_callStackList.InsertColumn(1, _T("State"), LVCFMT_LEFT, 150);
}

void CSequenceDebugPane::layoutControls(int cx, int cy)
{
    if (cx <= 0 || cy <= 0)
        return;

    // 3분할 레이아웃 (각각 1/3 높이)
    int sectionHeight = cy / 3;

    if (m_transitionList.GetSafeHwnd() != nullptr)
        m_transitionList.MoveWindow(0, 0, cx, sectionHeight);

    if (m_watchList.GetSafeHwnd() != nullptr)
        m_watchList.MoveWindow(0, sectionHeight, cx, sectionHeight);

    if (m_callStackList.GetSafeHwnd() != nullptr)
        m_callStackList.MoveWindow(0, sectionHeight * 2, cx, cy - sectionHeight * 2);
}

void CSequenceDebugPane::updateEngineStatus(uint32_t /*threadId*/, EngineStatus newStatus)
{
    m_currentStatus = newStatus;
}

void CSequenceDebugPane::updateDebugBreak(
    uint32_t /*threadId*/, const std::string& stateName,
    const std::map<std::string, ParamValue>& params,
    const std::chrono::system_clock::time_point& /*timestamp*/)
{
    if (m_watchList.GetSafeHwnd() == nullptr)
        return;

    // 변수 워치 리스트 갱신
    m_watchList.DeleteAllItems();

    int nRow = 0;
    for (const auto& pair : params)
    {
        m_watchList.InsertItem(nRow, CString(pair.first.c_str()));

        // ParamValue (variant) 값을 문자열로 변환
        CString strValue;
        CString strType;

        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>)
            {
                strValue.Format(_T("%d"), val);
                strType = _T("int");
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                strValue.Format(_T("%.3f"), val);
                strType = _T("double");
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                strValue = CString(val.c_str());
                strType = _T("string");
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                strValue = val ? _T("true") : _T("false");
                strType = _T("bool");
            }
        }, pair.second);

        m_watchList.SetItemText(nRow, 1, strValue);
        m_watchList.SetItemText(nRow, 2, strType);
        nRow++;
    }

    // 콜스택에 현재 상태 추가
    pushCallStack(stateName);
}

void CSequenceDebugPane::updateTransitionConditions(
    uint32_t /*threadId*/, const std::string& fromState, const std::string& toState,
    bool guardResult, const std::vector<InterlockCheckResult>& interlockResults)
{
    if (m_transitionList.GetSafeHwnd() == nullptr)
        return;

    // 기존 항목 중 동일한 전이를 찾아 갱신하거나 새로 추가
    CString strTransition;
    strTransition.Format(_T("%s -> %s"), CString(fromState.c_str()), CString(toState.c_str()));

    int nRow = -1;
    for (int i = 0; i < m_transitionList.GetItemCount(); i++)
    {
        if (m_transitionList.GetItemText(i, 0) == strTransition)
        {
            nRow = i;
            break;
        }
    }

    if (nRow < 0)
    {
        nRow = m_transitionList.GetItemCount();
        m_transitionList.InsertItem(nRow, strTransition);
    }

    m_transitionList.SetItemText(nRow, 1, guardResult ? _T("TRUE") : _T("FALSE"));

    // 인터락 결과를 문자열로 조합
    CString strInterlocks;
    for (const auto& result : interlockResults)
    {
        if (!strInterlocks.IsEmpty())
            strInterlocks += _T(", ");
        strInterlocks += CString(result.interlockName.c_str());
        strInterlocks += result.passed ? _T("=OK") : _T("=FAIL");
    }
    m_transitionList.SetItemText(nRow, 2, strInterlocks);
}

void CSequenceDebugPane::pushCallStack(const std::string& stateName)
{
    m_callStack.push_back(stateName);

    if (m_callStackList.GetSafeHwnd() == nullptr)
        return;

    int nRow = m_callStackList.GetItemCount();
    CString strIndex;
    strIndex.Format(_T("%d"), nRow);
    m_callStackList.InsertItem(nRow, strIndex);
    m_callStackList.SetItemText(nRow, 1, CString(stateName.c_str()));

    m_callStackList.EnsureVisible(nRow, FALSE);
}

void CSequenceDebugPane::clearCallStack()
{
    m_callStack.clear();
    if (m_callStackList.GetSafeHwnd() != nullptr)
        m_callStackList.DeleteAllItems();
}

} // namespace YggdraSeq
