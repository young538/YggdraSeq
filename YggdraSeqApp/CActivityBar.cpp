/**
 * @file CActivityBar.cpp
 * @brief VS Code Activity Bar 스타일 좌측 아이콘 사이드바 구현
 *
 * Owner Draw로 아이콘과 선택 표시를 직접 그립니다.
 * 텍스트 심볼로 아이콘을 표현합니다 (비트맵 없이도 동작).
 */

#include "pch.h"
#include "CActivityBar.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CActivityBar, CWnd)

BEGIN_MESSAGE_MAP(CActivityBar, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CActivityBar::CActivityBar()
    : m_currentMode(ActivityMode::Process)
{
}

CActivityBar::~CActivityBar()
{
}

BOOL CActivityBar::CreateBar(CWnd* pParentWnd, const CRect& rect)
{
    // WS_CHILD 윈도우로 생성
    CString strClass = AfxRegisterWndClass(
        CS_VREDRAW | CS_HREDRAW, ::LoadCursor(nullptr, IDC_ARROW));

    return CWnd::Create(strClass, _T("ActivityBar"),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rect, pParentWnd, 0);
}

void CActivityBar::setModeChangeCallback(std::function<void(ActivityMode)> callback)
{
    m_callback = std::move(callback);
}

BOOL CActivityBar::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;    // 깜박임 방지
}

void CActivityBar::OnPaint()
{
    CPaintDC dc(this);
    CRect clientRect;
    GetClientRect(clientRect);

    // 다크 배경 (#333333)
    dc.FillSolidRect(clientRect, RGB(0x33, 0x33, 0x33));

    // 각 아이콘 그리기
    for (int i = 0; i < ICON_COUNT; i++)
    {
        CRect iconRect = getIconRect(i);
        bool selected = false;

        switch (i)
        {
        case 0: selected = (m_currentMode == ActivityMode::Process); break;
        case 1: selected = (m_currentMode == ActivityMode::Debug); break;
        case 2: selected = (m_currentMode == ActivityMode::Settings); break;
        }

        drawIcon(&dc, iconRect, i, selected);
    }
}

void CActivityBar::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 클릭한 아이콘 판별
    for (int i = 0; i < ICON_COUNT; i++)
    {
        CRect iconRect = getIconRect(i);
        if (iconRect.PtInRect(point))
        {
            ActivityMode newMode = ActivityMode::Process;
            switch (i)
            {
            case 0: newMode = ActivityMode::Process; break;
            case 1: newMode = ActivityMode::Debug; break;
            case 2: newMode = ActivityMode::Settings; break;
            }

            if (m_currentMode != newMode)
            {
                m_currentMode = newMode;
                Invalidate();

                if (m_callback)
                    m_callback(newMode);
            }
            break;
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

CRect CActivityBar::getIconRect(int index) const
{
    int x = (BAR_WIDTH - ICON_SIZE) / 2;
    int y = ICON_SPACING + index * (ICON_SIZE + ICON_SPACING);
    return CRect(x, y, x + ICON_SIZE, y + ICON_SIZE);
}

void CActivityBar::drawIcon(CDC* pDC, const CRect& rect, int index, bool selected)
{
    // 선택 시: 좌측 테두리 강조 (VS Code 스타일)
    if (selected)
    {
        CRect barRect(0, rect.top - 2, 3, rect.bottom + 2);
        pDC->FillSolidRect(barRect, RGB(0xFF, 0xFF, 0xFF));
    }

    // 아이콘 색상: 선택 시 흰색, 미선택 시 회색
    COLORREF iconColor = selected ? RGB(0xFF, 0xFF, 0xFF) : RGB(0xCC, 0xCC, 0xCC);

    // 텍스트 기반 아이콘 심볼 (비트맵 없이도 동작)
    CFont font;
    font.CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI Symbol"));

    CFont* pOldFont = pDC->SelectObject(&font);
    pDC->SetTextColor(iconColor);
    pDC->SetBkMode(TRANSPARENT);

    CString strIcon;
    switch (index)
    {
    case 0: strIcon = _T("\x2261"); break;  // hamburger menu (공정)
    case 1: strIcon = _T("\x25CE"); break;  // bullseye (디버그)
    case 2: strIcon = _T("\x2699"); break;  // gear (설정)
    }

    pDC->DrawText(strIcon, CRect(rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);
}

} // namespace YggdraSeq
