/**
 * @file CSensorDataView.cpp
 * @brief 센서 데이터 실시간 테이블 뷰 구현
 *
 * 100ms 타이머로 SimulatedHardware의 값을 폴링하여 갱신합니다.
 */

#include "pch.h"
#include "CSensorDataView.h"
#include "SimulatedHardware.h"

namespace YggdraSeq
{

IMPLEMENT_DYNAMIC(CSensorDataView, CWnd)

BEGIN_MESSAGE_MAP(CSensorDataView, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CSensorDataView::CSensorDataView()
    : m_pHardware(nullptr)
    , m_prevFocus(0.0)
    , m_prevLight(0.0)
    , m_prevStageX(0.0)
    , m_prevStageY(0.0)
    , m_prevTheta(0.0)
    , m_prevArmX(300.0)
    , m_prevArmY(0.0)
{
}

CSensorDataView::~CSensorDataView()
{
}

BOOL CSensorDataView::CreateView(CWnd* pParentWnd, const CRect& rect)
{
    CString strClass = AfxRegisterWndClass(
        CS_VREDRAW | CS_HREDRAW, ::LoadCursor(nullptr, IDC_ARROW));

    return CWnd::Create(strClass, _T("SensorData"),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rect, pParentWnd, IDC_SENSOR_DATA_LIST);
}

void CSensorDataView::setHardware(SimulatedHardware* pHardware)
{
    m_pHardware = pHardware;
}

int CSensorDataView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
        | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL;

    if (!m_listCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_SENSOR_DATA_LIST + 100))
    {
        TRACE0("Failed to create Sensor Data CListCtrl\n");
        return -1;
    }

    m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    initializeList();

    // 100ms 갱신 타이머
    SetTimer(IDT_SENSOR_REFRESH, 100, nullptr);

    return 0;
}

void CSensorDataView::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (m_listCtrl.GetSafeHwnd() != nullptr)
        m_listCtrl.MoveWindow(0, 0, cx, cy);
}

void CSensorDataView::OnDestroy()
{
    KillTimer(IDT_SENSOR_REFRESH);
    CWnd::OnDestroy();
}

void CSensorDataView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_SENSOR_REFRESH)
    {
        refreshData();
    }
    CWnd::OnTimer(nIDEvent);
}

void CSensorDataView::initializeList()
{
    m_listCtrl.InsertColumn(0, _T("Sensor"), LVCFMT_LEFT, 120);
    m_listCtrl.InsertColumn(1, _T("Value"), LVCFMT_RIGHT, 80);
    m_listCtrl.InsertColumn(2, _T("Unit"), LVCFMT_CENTER, 50);
    m_listCtrl.InsertColumn(3, _T("Trend"), LVCFMT_CENTER, 50);

    // 센서 행 미리 생성
    const CString sensorNames[] = {
        _T("CameraFocus"), _T("LightIntensity"),
        _T("StageX"), _T("StageY"), _T("StageTheta"),
        _T("ArmX"), _T("ArmY"),
        _T("GripState"), _T("WaferCount"), _T("GoodCount"), _T("NgCount")
    };
    const CString units[] = {
        _T("mm"), _T("%"),
        _T("mm"), _T("mm"), _T("deg"),
        _T("mm"), _T("mm"),
        _T(""), _T("ea"), _T("ea"), _T("ea")
    };

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        m_listCtrl.InsertItem(i, sensorNames[i]);
        m_listCtrl.SetItemText(i, 1, _T("0"));
        m_listCtrl.SetItemText(i, 2, units[i]);
        m_listCtrl.SetItemText(i, 3, _T("-"));
    }
}

void CSensorDataView::updateRow(int row, const CString& /*name*/, double value,
    const CString& /*unit*/, double prevValue)
{
    CString strValue;
    strValue.Format(_T("%.2f"), value);
    m_listCtrl.SetItemText(row, 1, strValue);

    // 트렌드 화살표
    CString strTrend;
    if (std::abs(value - prevValue) < 0.01)
        strTrend = _T("-");
    else if (value > prevValue)
        strTrend = _T("\x2191");  // up arrow
    else
        strTrend = _T("\x2193");  // down arrow

    m_listCtrl.SetItemText(row, 3, strTrend);
}

void CSensorDataView::refreshData()
{
    if (m_pHardware == nullptr || m_listCtrl.GetSafeHwnd() == nullptr)
        return;

    double focus = m_pHardware->getCameraFocus();
    double light = m_pHardware->getLightIntensity();
    double stgX = m_pHardware->getStageX();
    double stgY = m_pHardware->getStageY();
    double theta = m_pHardware->getStageTheta();
    double armX = m_pHardware->getArmX();
    double armY = m_pHardware->getArmY();

    updateRow(0, _T("CameraFocus"), focus, _T("mm"), m_prevFocus);
    updateRow(1, _T("LightIntensity"), light, _T("%"), m_prevLight);
    updateRow(2, _T("StageX"), stgX, _T("mm"), m_prevStageX);
    updateRow(3, _T("StageY"), stgY, _T("mm"), m_prevStageY);
    updateRow(4, _T("StageTheta"), theta, _T("deg"), m_prevTheta);
    updateRow(5, _T("ArmX"), armX, _T("mm"), m_prevArmX);
    updateRow(6, _T("ArmY"), armY, _T("mm"), m_prevArmY);

    // Grip 상태 (bool -> 텍스트)
    m_listCtrl.SetItemText(7, 1, m_pHardware->getGripState() ? _T("HOLD") : _T("OPEN"));
    m_listCtrl.SetItemText(7, 3, _T("-"));

    // 카운터 (int)
    CString strVal;
    strVal.Format(_T("%d"), m_pHardware->getWaferCount());
    m_listCtrl.SetItemText(8, 1, strVal);
    m_listCtrl.SetItemText(8, 3, _T("-"));

    strVal.Format(_T("%d"), m_pHardware->getGoodCount());
    m_listCtrl.SetItemText(9, 1, strVal);

    strVal.Format(_T("%d"), m_pHardware->getNgCount());
    m_listCtrl.SetItemText(10, 1, strVal);

    // 이전 값 저장
    m_prevFocus = focus;
    m_prevLight = light;
    m_prevStageX = stgX;
    m_prevStageY = stgY;
    m_prevTheta = theta;
    m_prevArmX = armX;
    m_prevArmY = armY;
}

} // namespace YggdraSeq
