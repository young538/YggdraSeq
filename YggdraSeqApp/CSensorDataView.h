/**
 * @file CSensorDataView.h
 * @brief 센서 데이터 실시간 테이블 뷰
 *
 * 외관검사 장비의 센서 데이터를 CListCtrl 테이블로 실시간 표시합니다.
 *
 * 표시 센서:
 *   CameraFocus, LightIntensity, StageX, StageY, StageTheta,
 *   ArmX, ArmY, GripForce, WaferCount, GoodCount, NgCount
 *
 * 컬럼:
 *   센서 이름 | 현재 값 | 단위 | 상태 | 트렌드
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include <string>

namespace YggdraSeq
{

// 전방 선언
class SimulatedHardware;

/**
 * @class CSensorDataView
 * @brief 센서 데이터 실시간 테이블 (CWnd + CListCtrl)
 *
 * 타이머(100ms)로 주기적으로 SimulatedHardware의 값을 읽어 갱신합니다.
 */
class CSensorDataView : public CWnd
{
    DECLARE_DYNAMIC(CSensorDataView)

public:
    CSensorDataView();
    virtual ~CSensorDataView();

    /**
     * @brief 센서 데이터 뷰 생성
     * @param pParentWnd 부모 윈도우
     * @param rect       초기 영역
     * @return TRUE = 성공
     */
    BOOL CreateView(CWnd* pParentWnd, const CRect& rect);

    /**
     * @brief SimulatedHardware 참조 설정
     * @param pHardware 시뮬레이션 하드웨어 (관찰용)
     */
    void setHardware(SimulatedHardware* pHardware);

    /**
     * @brief 센서 데이터를 수동 갱신
     */
    void refreshData();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    void initializeList();
    void updateRow(int row, const CString& name, double value, const CString& unit,
        double prevValue);

    CListCtrl m_listCtrl;               ///< 센서 데이터 리스트 컨트롤
    SimulatedHardware* m_pHardware;     ///< 시뮬레이션 하드웨어

    // 이전 값 (트렌드 화살표 계산용)
    double m_prevFocus;
    double m_prevLight;
    double m_prevStageX;
    double m_prevStageY;
    double m_prevTheta;
    double m_prevArmX;
    double m_prevArmY;

    static constexpr int SENSOR_COUNT = 11;  ///< 센서 수
};

} // namespace YggdraSeq
