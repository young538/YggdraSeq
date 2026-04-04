/**
 * @file CThreadMonitorPane.h
 * @brief 스레드 모니터 도킹 패널
 *
 * 멀티스레드 실행 상태를 실시간으로 모니터링합니다.
 * 각 워커 스레드의 상태, 실행 중인 작업, 동기화 객체 점유 상태를 표시합니다.
 * 순환 대기(데드락) 패턴 감지 시 빨간색 하이라이트로 경고합니다.
 *
 * 표시 항목:
 *   - 스레드 ID (OS 스레드 ID)
 *   - 스레드 이름 (예: "Inspection", "Handler")
 *   - 상태 (Running / Waiting / Blocked / Idle / Terminated)
 *   - 락 상태 (보유/대기 중인 동기화 객체)
 *   - 마지막 활동 시간
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/Types.h"
#include <chrono>
#include <string>
#include <map>

namespace YggdraSeq
{

/**
 * @class CThreadMonitorPane
 * @brief 스레드 모니터 도킹 패널 (CDockablePane 파생)
 *
 * 데드락 감지 로직:
 *   스레드 A가 락 X를 보유하고 락 Y를 대기 중이고,
 *   스레드 B가 락 Y를 보유하고 락 X를 대기 중이면 데드락으로 판정합니다.
 *   간단한 2-스레드 순환 감지만 구현합니다.
 */
class AFX_EXT_CLASS CThreadMonitorPane : public CWnd
{
    DECLARE_DYNAMIC(CThreadMonitorPane)

public:
    CThreadMonitorPane();
    virtual ~CThreadMonitorPane();

    // ========================================================================
    // 갱신 메서드 (UI 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 스레드 상태 갱신
     *
     * @param threadId   스레드 ID
     * @param threadName 스레드 이름
     * @param newState   새로운 스레드 상태
     * @param lockInfo   보유/대기 중인 동기화 객체 정보
     * @param timestamp  상태 변경 시간
     */
    void updateThreadState(uint32_t threadId, const std::string& threadName,
        ThreadState newState, const std::string& lockInfo,
        const std::chrono::system_clock::time_point& timestamp);

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void initializeListColumns();
    int findOrCreateRow(uint32_t threadId);
    bool detectDeadlock() const;

    CListCtrl m_listCtrl;  ///< 스레드 정보 리스트 컨트롤

    /**
     * @struct ThreadMonitorInfo
     * @brief 스레드별 모니터링 정보
     */
    struct ThreadMonitorInfo
    {
        std::string threadName;                                 ///< 스레드 이름
        ThreadState state = ThreadState::Idle;                  ///< 현재 상태
        std::string lockInfo;                                   ///< 락 정보
        std::chrono::system_clock::time_point lastActivity;     ///< 마지막 활동 시간
        int listIndex = -1;                                     ///< 리스트 행 인덱스
        bool deadlockSuspect = false;                           ///< 데드락 의심 플래그
    };

    std::map<uint32_t, ThreadMonitorInfo> m_threadMap;  ///< 스레드 모니터링 맵
};

} // namespace YggdraSeq
