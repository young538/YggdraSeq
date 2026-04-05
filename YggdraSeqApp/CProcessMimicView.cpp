/**
 * @file CProcessMimicView.cpp
 * @brief 외관검사 장비 미믹 다이어그램 구현
 *
 * Owner Draw로 장비 레이아웃을 그리고 50ms 타이머로 애니메이션합니다.
 *
 * 레이아웃:
 *   좌측 절반: Inspection 영역 (LDR -> STG -> CAM+LED -> 결과)
 *   우측 절반: Handler 영역 (IN -> ARM -> OUT)
 */

#include "pch.h"
#include "CProcessMimicView.h"
#include "SimulatedHardware.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CProcessMimicView, CWnd)

BEGIN_MESSAGE_MAP(CProcessMimicView, CWnd)
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CProcessMimicView::CProcessMimicView()
    : m_pHardware(nullptr)
    , m_animationFrame(0)
    , m_pulseState(false)
{
}

CProcessMimicView::~CProcessMimicView()
{
}

BOOL CProcessMimicView::CreateView(CWnd* pParentWnd, const CRect& rect)
{
    CString strClass = AfxRegisterWndClass(
        CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW));

    return CWnd::Create(strClass, _T("ProcessMimic"),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rect, pParentWnd, IDC_PROCESS_MIMIC);
}

int CProcessMimicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 50ms 애니메이션 타이머 시작
    SetTimer(IDT_MIMIC_ANIMATION, 50, nullptr);
    return 0;
}

void CProcessMimicView::OnDestroy()
{
    KillTimer(IDT_MIMIC_ANIMATION);
    CWnd::OnDestroy();
}

void CProcessMimicView::setHardware(SimulatedHardware* pHardware)
{
    m_pHardware = pHardware;
}

void CProcessMimicView::setActiveStates(const std::string& inspectionState,
    const std::string& handlerState)
{
    m_inspectionState = inspectionState;
    m_handlerState = handlerState;
}

void CProcessMimicView::setInterlockViolation(const std::string& deviceName)
{
    m_violationDevice = deviceName;
}

BOOL CProcessMimicView::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;    // 깜박임 방지
}

void CProcessMimicView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_MIMIC_ANIMATION)
    {
        m_animationFrame++;
        m_pulseState = (m_animationFrame % 10) < 5;  // 0.5초 주기 깜박임
        Invalidate(FALSE);
    }

    CWnd::OnTimer(nIDEvent);
}

void CProcessMimicView::OnPaint()
{
    CPaintDC dc(this);
    CRect clientRect;
    GetClientRect(clientRect);

    // 더블 버퍼링으로 깜박임 방지
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);

    drawMimic(&memDC, clientRect);

    dc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);
}

void CProcessMimicView::drawMimic(CDC* pDC, const CRect& clientRect)
{
    // 다크 배경
    pDC->FillSolidRect(clientRect, RGB(25, 25, 25));

    // 좌측: Inspection 영역
    CRect inspRect(clientRect.left + 10, clientRect.top + 10,
        clientRect.CenterPoint().x - 5, clientRect.bottom - 10);
    drawInspectionArea(pDC, inspRect);

    // 우측: Handler 영역
    CRect handlerRect(clientRect.CenterPoint().x + 5, clientRect.top + 10,
        clientRect.right - 10, clientRect.bottom - 10);
    drawHandlerArea(pDC, handlerRect);
}

void CProcessMimicView::drawInspectionArea(CDC* pDC, const CRect& areaRect)
{
    // 영역 테두리
    CPen penBorder(PS_SOLID, 1, RGB(80, 80, 80));
    CPen* pOldPen = pDC->SelectObject(&penBorder);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(areaRect);
    pDC->SelectObject(pOldPen);

    // 영역 제목
    pDC->SetTextColor(RGB(100, 180, 255));
    pDC->SetBkMode(TRANSPARENT);
    CFont titleFont;
    titleFont.CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
    CFont* pOldFont = pDC->SelectObject(&titleFont);
    pDC->TextOut(areaRect.left + 5, areaRect.top + 2, _T("Inspection"));
    pDC->SelectObject(pOldFont);

    if (m_pHardware == nullptr)
        return;

    int cx = areaRect.Width();
    int cy = areaRect.Height();
    int left = areaRect.left;
    int top = areaRect.top + 20;
    int devW = cx / 3 - 10;
    int devH = 50;

    // Loader (LDR)
    CString strLdr;
    strLdr.Format(_T("Wafer: %d"), m_pHardware->getWaferCount());
    bool ldrActive = (m_inspectionState == "Load");
    drawDevice(pDC, CRect(left + 10, top + 10, left + 10 + devW, top + 10 + devH),
        _T("LDR"), strLdr, RGB(60, 120, 200), ldrActive, m_violationDevice == "LDR");

    // Stage (STG)
    CString strStg;
    strStg.Format(_T("X:%.0f Y:%.0f"), m_pHardware->getStageX(), m_pHardware->getStageY());
    bool stgActive = (m_inspectionState == "Align");
    drawDevice(pDC, CRect(left + 10, top + 80, left + 10 + devW, top + 80 + devH),
        _T("STG"), strStg, RGB(60, 180, 120), stgActive, m_violationDevice == "STG");

    // Camera (CAM)
    CString strCam;
    strCam.Format(_T("Focus: %.1fmm"), m_pHardware->getCameraFocus());
    bool camActive = (m_inspectionState == "Capture");
    drawDevice(pDC, CRect(left + 20 + devW, top + 10, left + 20 + devW * 2, top + 10 + devH),
        _T("CAM"), strCam, RGB(200, 120, 60), camActive, m_violationDevice == "CAM");

    // Light (LED)
    CString strLed;
    strLed.Format(_T("Light: %.0f%%"), m_pHardware->getLightIntensity());
    bool ledActive = (m_inspectionState == "Capture");
    drawDevice(pDC, CRect(left + 20 + devW, top + 80, left + 20 + devW * 2, top + 80 + devH),
        _T("LED"), strLed, RGB(255, 220, 50), ledActive, m_violationDevice == "LED");

    // 연결선 (흐름: LDR → CAM, LDR → STG, STG → LED, CAM → LED)
    // LDR 오른쪽 → CAM 왼쪽 (수평)
    drawConnection(pDC, CPoint(left + 10 + devW, top + 10 + devH / 2),
        CPoint(left + 20 + devW, top + 10 + devH / 2), ldrActive);
    // LDR 아래 → STG 위 (수직)
    drawConnection(pDC, CPoint(left + 10 + devW / 2, top + 10 + devH),
        CPoint(left + 10 + devW / 2, top + 80), stgActive);
    // STG 오른쪽 → LED 왼쪽 (수평)
    drawConnection(pDC, CPoint(left + 10 + devW, top + 80 + devH / 2),
        CPoint(left + 20 + devW, top + 80 + devH / 2), ledActive);
    // CAM 아래 → LED 위 (수직)
    drawConnection(pDC, CPoint(left + 20 + devW + devW / 2, top + 10 + devH),
        CPoint(left + 20 + devW + devW / 2, top + 80), camActive);

    // 검사 결과 카운터
    CRect counterRect(left + 30 + devW * 2, top + 10, areaRect.right - 5, top + 10 + devH * 2 + 30);
    drawCounter(pDC, counterRect);

    // Inspect 상태 표시
    if (m_inspectionState == "Inspect" || m_inspectionState == "Judge")
    {
        CString strState;
        strState.Format(_T("[%s]"), CString(m_inspectionState.c_str()));
        pDC->SetTextColor(RGB(255, 200, 100));
        pDC->TextOut(left + cx / 2 - 30, top + cy - 40, strState);
    }
}

void CProcessMimicView::drawHandlerArea(CDC* pDC, const CRect& areaRect)
{
    CPen penBorder(PS_SOLID, 1, RGB(80, 80, 80));
    CPen* pOldPen = pDC->SelectObject(&penBorder);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(areaRect);
    pDC->SelectObject(pOldPen);

    pDC->SetTextColor(RGB(255, 150, 100));
    pDC->SetBkMode(TRANSPARENT);
    CFont titleFont;
    titleFont.CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
    CFont* pOldFont = pDC->SelectObject(&titleFont);
    pDC->TextOut(areaRect.left + 5, areaRect.top + 2, _T("Handler"));
    pDC->SelectObject(pOldFont);

    if (m_pHardware == nullptr)
        return;

    int cx = areaRect.Width();
    int top = areaRect.top + 20;
    int left = areaRect.left;
    int devW = cx / 3 - 10;
    int devH = 50;

    // Robot Arm (ARM)
    CString strArm;
    strArm.Format(_T("X:%.0f Y:%.0f"), m_pHardware->getArmX(), m_pHardware->getArmY());
    bool armActive = (m_handlerState == "Pick" || m_handlerState == "Move" || m_handlerState == "Return");
    drawDevice(pDC, CRect(left + 10 + devW, top + 10, left + 10 + devW * 2, top + 10 + devH),
        _T("ARM"), strArm, RGB(180, 80, 200), armActive, m_violationDevice == "ARM");

    // Grip 상태
    CString strGrip = m_pHardware->getGripState() ? _T("Grip: HOLD") : _T("Grip: OPEN");
    COLORREF gripColor = m_pHardware->getGripState() ? RGB(255, 220, 50) : RGB(128, 128, 128);
    drawDevice(pDC, CRect(left + 10 + devW, top + 70, left + 10 + devW * 2, top + 70 + 35),
        _T(""), strGrip, gripColor, false, false);

    // Input Cassette (IN)
    CString strIn;
    strIn.Format(_T("IN: %d"), m_pHardware->getWaferCount());
    drawDevice(pDC, CRect(left + 10, top + 30, left + 10 + devW, top + 30 + devH),
        _T("IN"), strIn, RGB(100, 160, 220), false, false);

    // Output Cassette (OUT)
    CString strOut;
    strOut.Format(_T("G:%d N:%d"), m_pHardware->getGoodCount(), m_pHardware->getNgCount());
    drawDevice(pDC, CRect(left + 20 + devW * 2, top + 30, areaRect.right - 5, top + 30 + devH),
        _T("OUT"), strOut, RGB(100, 200, 100), false, false);

    // 연결선 (IN -> ARM -> OUT)
    drawConnection(pDC, CPoint(left + 10 + devW, top + 55),
        CPoint(left + 10 + devW, top + 35), m_handlerState == "Pick");
    drawConnection(pDC, CPoint(left + 10 + devW * 2, top + 35),
        CPoint(left + 20 + devW * 2, top + 55), m_handlerState == "Move");
}

void CProcessMimicView::drawDevice(CDC* pDC, const CRect& rect, const CString& name,
    const CString& value, COLORREF color, bool active, bool violation)
{
    // 배경
    COLORREF bgColor = RGB(40, 40, 40);
    if (active && m_pulseState)
    {
        // 깜박임: 밝은 배경
        bgColor = RGB(60, 60, 80);
    }
    pDC->FillSolidRect(rect, bgColor);

    // 테두리
    COLORREF borderColor = violation ? RGB(255, 50, 50) : color;
    int borderWidth = violation ? 3 : 1;
    CPen pen(PS_SOLID, borderWidth, borderColor);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(rect);
    pDC->SelectObject(pOldPen);

    // 장치 이름 (상단)
    if (!name.IsEmpty())
    {
        CFont nameFont;
        nameFont.CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
        CFont* pOldFont = pDC->SelectObject(&nameFont);
        pDC->SetTextColor(color);
        pDC->TextOut(rect.left + 4, rect.top + 3, name);
        pDC->SelectObject(pOldFont);
    }

    // 값 (하단)
    CFont valFont;
    valFont.CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Consolas"));
    CFont* pOldFont = pDC->SelectObject(&valFont);
    pDC->SetTextColor(RGB(200, 200, 200));
    CRect valRect = rect;
    valRect.top += name.IsEmpty() ? 5 : 18;
    valRect.left += 4;
    pDC->DrawText(value, valRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);
}

void CProcessMimicView::drawCounter(CDC* pDC, const CRect& rect)
{
    if (m_pHardware == nullptr)
        return;

    pDC->FillSolidRect(rect, RGB(35, 35, 35));

    CPen pen(PS_SOLID, 1, RGB(80, 80, 80));
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(rect);
    pDC->SelectObject(pOldPen);

    CFont font;
    font.CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
    CFont* pOldFont = pDC->SelectObject(&font);

    int y = rect.top + 5;

    pDC->SetTextColor(RGB(200, 200, 200));
    pDC->TextOut(rect.left + 5, y, _T("Result"));

    y += 20;
    CString strGood;
    strGood.Format(_T("Good: %d"), m_pHardware->getGoodCount());
    pDC->SetTextColor(RGB(80, 220, 80));
    pDC->TextOut(rect.left + 5, y, strGood);

    y += 18;
    CString strNg;
    strNg.Format(_T("NG:   %d"), m_pHardware->getNgCount());
    pDC->SetTextColor(RGB(255, 80, 80));
    pDC->TextOut(rect.left + 5, y, strNg);

    // 수율
    int total = m_pHardware->getGoodCount() + m_pHardware->getNgCount();
    if (total > 0)
    {
        y += 18;
        double yield = 100.0 * m_pHardware->getGoodCount() / total;
        CString strYield;
        strYield.Format(_T("Yield: %.1f%%"), yield);
        pDC->SetTextColor(RGB(200, 200, 200));
        pDC->TextOut(rect.left + 5, y, strYield);
    }

    pDC->SelectObject(pOldFont);
}

void CProcessMimicView::drawConnection(CDC* pDC, CPoint from, CPoint to, bool active)
{
    COLORREF color = active ? RGB(100, 180, 255) : RGB(60, 60, 60);
    int width = active ? 2 : 1;
    CPen pen(PS_SOLID, width, color);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->MoveTo(from);
    pDC->LineTo(to);
    pDC->SelectObject(pOldPen);
}

void CProcessMimicView::drawWafer(CDC* pDC, CPoint pos, bool moving)
{
    COLORREF color = moving ? RGB(100, 200, 255) : RGB(200, 200, 200);
    CBrush brush(color);
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    pDC->Ellipse(pos.x - 8, pos.y - 8, pos.x + 8, pos.y + 8);
    pDC->SelectObject(pOldBrush);
}

} // namespace YggdraSeq
