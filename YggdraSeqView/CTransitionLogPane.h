/**
 * @file CTransitionLogPane.h
 * @brief 전이 이력 도킹 패널
 *
 * 모든 상태 전이를 시간순으로 기록하고 분석할 수 있는 패널입니다.
 * CDockablePane에서 파생하며, 내부에 CListCtrl을 포함합니다.
 *
 * 표시 항목:
 *   - 시간 (밀리초 단위)
 *   - 스레드 ID
 *   - From 상태
 *   - To 상태
 *   - Guard 결과 (true/false)
 *   - Interlock 결과 (각 인터락별 Pass/Fail)
 *   - 비고 (실패 사유)
 *
 * 분석 기능:
 *   - 전이 실패 필터 (실패한 전이만 표시)
 *   - 스레드별 필터
 *   - CSV 내보내기
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/Types.h"
#include <chrono>
#include <string>
#include <vector>
#include <deque>

namespace YggdraSeq
{

/**
 * @struct TransitionRecord
 * @brief 전이 이력 레코드
 */
struct TransitionRecord
{
    uint32_t threadId;                              ///< 전이 수행 스레드 ID
    std::string fromState;                          ///< 출발 상태
    std::string toState;                            ///< 도착 상태
    bool guardResult;                               ///< Guard 조건 결과
    std::vector<InterlockCheckResult> interlockResults;  ///< 인터락 결과
    std::chrono::system_clock::time_point timestamp; ///< 발생 시간
    bool overallSuccess;                            ///< 전체 성공 여부
};

/**
 * @class CTransitionLogPane
 * @brief 전이 이력 도킹 패널 (CDockablePane 파생)
 *
 * 전이 이력을 시간순으로 기록하고, 필터링 및 CSV 내보내기를 제공합니다.
 */
class AFX_EXT_CLASS CTransitionLogPane : public CWnd
{
    DECLARE_DYNAMIC(CTransitionLogPane)

public:
    CTransitionLogPane();
    virtual ~CTransitionLogPane();

    // ========================================================================
    // 갱신 메서드 (UI 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 전이 기록 추가
     *
     * @param threadId         스레드 ID
     * @param fromState        출발 상태
     * @param toState          도착 상태
     * @param guardResult      Guard 조건 결과
     * @param interlockResults 인터락 체크 결과 목록
     * @param timestamp        전이 시간
     */
    void addTransitionRecord(uint32_t threadId, const std::string& fromState,
        const std::string& toState, bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults,
        const std::chrono::system_clock::time_point& timestamp);

    /**
     * @brief 모든 전이 기록 삭제
     */
    void clearRecords();

    // ========================================================================
    // 필터링
    // ========================================================================

    /**
     * @brief 실패한 전이만 표시 필터
     * @param failedOnly true = 실패한 전이만 표시
     */
    void setFailedOnlyFilter(bool failedOnly);

    /**
     * @brief 스레드 필터 설정
     * @param threadId 특정 스레드만 (0 = 전체)
     */
    void setThreadFilter(uint32_t threadId);

    /**
     * @brief CSV 파일로 내보내기
     * @param filePath 저장 경로
     * @return true = 성공
     */
    bool exportToCsv(const std::string& filePath) const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void initializeListColumns();
    void rebuildFilteredList();
    bool passesFilter(const TransitionRecord& record) const;

    CListCtrl m_listCtrl;                       ///< 전이 이력 리스트 컨트롤
    std::deque<TransitionRecord> m_allRecords;  ///< 전체 전이 기록

    // 필터 상태
    bool m_failedOnlyFilter;    ///< 실패만 표시 필터
    uint32_t m_threadFilter;    ///< 스레드 필터 (0 = 전체)

    static constexpr int MAX_RECORDS = 5000;  ///< 최대 기록 수
};

} // namespace YggdraSeq
