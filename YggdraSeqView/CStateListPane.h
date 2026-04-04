/**
 * @file CStateListPane.h
 * @brief 상태 목록 도킹 패널
 *
 * 현재 실행 중인 상태와 엔진 정보를 리스트로 표시합니다.
 * CDockablePane에서 파생하며, 내부에 CListCtrl을 포함합니다.
 *
 * 표시 항목:
 *   - 엔진 이름 (Inspection / Handler)
 *   - 현재 상태 이름
 *   - 상태 유형 (Init / Process / Wait / End)
 *   - 엔진 상태 (Running / Paused / Error 등)
 *   - 실행 스레드 ID
 *   - 진입 시간
 *   - 경과 시간
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
 * @class CStateListPane
 * @brief 상태 목록 도킹 패널 (CDockablePane 파생)
 *
 * CSeqMonitorFrame에서 UI 스레드 메시지를 받아 갱신됩니다.
 * 각 엔진(스레드)의 현재 상태를 한 행으로 표시합니다.
 */
class AFX_EXT_CLASS CStateListPane : public CDockablePane
{
    DECLARE_DYNAMIC(CStateListPane)

public:
    CStateListPane();
    virtual ~CStateListPane();

    // ========================================================================
    // 갱신 메서드 (CSeqMonitorFrame의 UI 스레드 핸들러에서 호출)
    // ========================================================================

    /**
     * @brief 상태 전이 발생 시 목록 갱신
     *
     * @param threadId  전이를 수행한 스레드 ID
     * @param fromState 출발 상태 이름
     * @param toState   도착 상태 이름
     * @param timestamp 전이 발생 시간
     */
    void updateStateChange(uint32_t threadId, const std::string& fromState,
        const std::string& toState,
        const std::chrono::system_clock::time_point& timestamp);

    /**
     * @brief 엔진 상태 변경 시 갱신
     *
     * @param threadId  스레드 ID
     * @param newStatus 새로운 엔진 상태
     */
    void updateEngineStatus(uint32_t threadId, EngineStatus newStatus);

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()

private:
    /**
     * @brief CListCtrl의 컬럼을 초기화
     */
    void initializeListColumns();

    /**
     * @brief 특정 스레드의 행을 찾거나 새로 생성
     * @param threadId 스레드 ID
     * @return 리스트 행 인덱스
     */
    int findOrCreateRow(uint32_t threadId);

    CListCtrl m_listCtrl;  ///< 상태 정보 리스트 컨트롤

    /**
     * @struct ThreadInfo
     * @brief 스레드별 상태 정보를 추적하는 구조체
     */
    struct ThreadInfo
    {
        std::string currentState;                           ///< 현재 상태 이름
        EngineStatus engineStatus = EngineStatus::Idle;     ///< 엔진 상태
        std::chrono::system_clock::time_point entryTime;    ///< 현재 상태 진입 시간
        int listIndex = -1;                                 ///< 리스트 행 인덱스
    };

    std::map<uint32_t, ThreadInfo> m_threadInfoMap;  ///< 스레드별 정보 맵
};

} // namespace YggdraSeq
