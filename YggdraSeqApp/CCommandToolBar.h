/**
 * @file CCommandToolBar.h
 * @brief 상단 명령 버튼 바
 *
 * CWnd에서 파생하여 시퀀스 제어 버튼과 모드 전환 버튼을 배치합니다.
 * CMFCToolBar 대신 일반 CButton을 사용하여 직접 그립니다.
 *
 * 구성:
 *   [Run] [Pause] [Stop]  |  [EMO]  |  [Auto] [Manual] [Debug]
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/Types.h"

namespace YggdraSeq
{

/**
 * @enum RunMode
 * @brief 시퀀스 운전 모드
 */
enum class RunMode
{
    Auto,       ///< 자동 실행 (전체 시퀀스 자동 진행)
    Manual,     ///< 수동 모드 (스텝별 수동 진행)
    Debug       ///< 디버그 모드 (브레이크포인트 활성)
};

// 버튼 바 높이 상수
constexpr int COMMAND_BAR_HEIGHT = 32;

/**
 * @class CCommandToolBar
 * @brief 상단 명령 버튼 바 (CWnd 파생)
 *
 * 시퀀스 제어 버튼(Run/Pause/Stop/EMO)과 모드 선택 버튼(Auto/Manual/Debug)을
 * 일반 CButton으로 배치합니다. 부모 프레임에 WM_COMMAND를 전달합니다.
 */
class CCommandToolBar : public CWnd
{
    DECLARE_DYNAMIC(CCommandToolBar)

public:
    CCommandToolBar();
    virtual ~CCommandToolBar();

    /**
     * @brief 버튼 바 생성
     * @param pParentWnd 부모 윈도우
     * @return TRUE = 성공
     */
    BOOL CreateToolBar(CWnd* pParentWnd);

    /**
     * @brief 엔진 상태에 따라 LED 인디케이터 색상 갱신
     * @param status 엔진 상태
     */
    void updateLedIndicator(EngineStatus status);

    /** @brief 현재 운전 모드 반환 */
    RunMode getCurrentMode() const { return m_currentMode; }

    /** @brief 운전 모드 설정 */
    void setRunMode(RunMode mode) { m_currentMode = mode; }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnButtonClicked(UINT nID);

    DECLARE_MESSAGE_MAP()

private:
    /** @brief 버튼 위치 재배치 */
    void layoutButtons();

    // 시퀀스 제어 버튼
    CButton m_btnRun;
    CButton m_btnPause;
    CButton m_btnStop;
    CButton m_btnEmo;

    // 모드 버튼
    CButton m_btnAuto;
    CButton m_btnManual;
    CButton m_btnDebug;

    RunMode m_currentMode;          ///< 현재 운전 모드
    EngineStatus m_currentStatus;   ///< 현재 엔진 상태 (LED용)
};

} // namespace YggdraSeq
