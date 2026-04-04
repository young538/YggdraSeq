/**
 * @file MainFrm.h
 * @brief CFrameWndEx 메인 프레임
 *
 * VS Code 스타일 HMI 레이아웃을 구현합니다.
 * Activity Bar(좌측) + 메인 탭(중앙) + 하단 패널 + Status Bar 구조.
 * View DLL의 CSeqMonitorFrame을 통합하여 모니터링 패널을 배치합니다.
 *
 * 시퀀스 실행:
 *   Inspection(Thread-A)과 Handler(Thread-B) 두 SequenceEngine을 생성하고,
 *   CSeqMonitorFrame을 Observer로 등록하여 실시간 모니터링합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "CActivityBar.h"
#include "CCommandToolBar.h"
#include "CProcessMimicView.h"
#include "CSensorDataView.h"
#include "SimulatedHardware.h"
#include "../YggdraSeqCore/SequenceEngine.h"
#include "../YggdraSeqView/CSeqMonitorFrame.h"

namespace YggdraSeq
{

// 전방 선언
class InspectState;

/**
 * @class CMainFrame
 * @brief 메인 프레임 (CFrameWndEx 파생, VS Code 스타일 레이아웃)
 *
 * 두 개의 SequenceEngine(Inspection, Handler)을 관리하고,
 * View DLL의 도킹 패널들을 하단/우측에 배치합니다.
 * 상단에 CCommandToolBar, 좌측에 CActivityBar를 배치합니다.
 */
class CMainFrame : public CFrameWndEx
{
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();
    virtual ~CMainFrame();

protected:
    // MFC 오버라이드
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    // 시퀀스 제어 명령 핸들러
    afx_msg void OnSeqRun();
    afx_msg void OnSeqPause();
    afx_msg void OnSeqStop();
    afx_msg void OnSeqEmo();
    afx_msg void OnSeqModeAuto();
    afx_msg void OnSeqModeManual();
    afx_msg void OnSeqModeDebug();

    // UI 갱신 명령 핸들러
    afx_msg void OnUpdateSeqRun(CCmdUI* pCmdUI);
    afx_msg void OnUpdateSeqPause(CCmdUI* pCmdUI);
    afx_msg void OnUpdateSeqStop(CCmdUI* pCmdUI);

    // 커스텀 메시지 핸들러
    afx_msg LRESULT OnHardwareUpdated(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // ========================================================================
    // 초기화 메서드
    // ========================================================================

    /** @brief 시퀀스 엔진 및 상태 초기화 */
    void initializeSequences();

    /** @brief 인터락 설정 */
    void setupInterlocks();

    /** @brief UI 레이아웃 배치 */
    void layoutChildren();

    /** @brief 상태 바 갱신 */
    void updateStatusBar();

    // ========================================================================
    // 멤버 변수
    // ========================================================================

    // --- UI 컴포넌트 ---
    CActivityBar m_activityBar;         ///< 좌측 Activity Bar
    CCommandToolBar m_commandToolBar;   ///< 상단 명령 툴바
    CProcessMimicView m_mimicView;      ///< 미믹 다이어그램
    CSensorDataView m_sensorView;       ///< 센서 데이터 테이블
    CMFCStatusBar m_statusBar;          ///< 하단 상태 바

    // --- View DLL 통합 ---
    CSeqMonitorFrame* m_pMonitorFrame;  ///< View DLL 모니터 프레임 (동적 생성)

    // --- 시퀀스 엔진 ---
    std::unique_ptr<SequenceEngine> m_pInspectionEngine;    ///< 검사 엔진 (Thread-A)
    std::unique_ptr<SequenceEngine> m_pHandlerEngine;       ///< 핸들러 엔진 (Thread-B)

    // --- 시뮬레이션 하드웨어 ---
    std::unique_ptr<SimulatedHardware> m_pHardware;         ///< 공유 가상 하드웨어

    // --- InspectState 참조 (JudgeState에서 결과 조회용) ---
    InspectState* m_pInspectState;

    // --- 초기화 상태 ---
    bool m_sequencesInitialized;    ///< 시퀀스 초기화 완료 플래그
};

} // namespace YggdraSeq
