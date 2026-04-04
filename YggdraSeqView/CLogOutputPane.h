/**
 * @file CLogOutputPane.h
 * @brief 실시간 로그 출력 도킹 패널
 *
 * 실시간 로그 메시지를 CListCtrl에 표시합니다.
 * 스레드별/레벨별 필터링, 키워드 검색, 자동 스크롤, 파일 내보내기를 지원합니다.
 *
 * 표시 항목:
 *   - 시간 (밀리초 단위)
 *   - 스레드 ID
 *   - 레벨 (TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL)
 *   - 소스
 *   - 메시지
 *
 * 필터링 기능:
 *   - 로그 레벨별 필터 (특정 레벨 이상만 표시)
 *   - 스레드 ID별 필터 (특정 스레드만 표시)
 *   - 키워드 검색
 *   - 자동 스크롤 on/off
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
#include <set>

namespace YggdraSeq
{

/**
 * @struct LogEntry
 * @brief 로그 항목 데이터
 */
struct LogEntry
{
    uint32_t threadId;                              ///< 스레드 ID
    LogLevel level;                                 ///< 로그 레벨
    std::string source;                             ///< 로그 발생 소스
    std::string message;                            ///< 로그 메시지
    std::chrono::system_clock::time_point timestamp; ///< 발생 시간
};

/**
 * @class CLogOutputPane
 * @brief 실시간 로그 출력 도킹 패널 (CDockablePane 파생)
 *
 * 로그 메시지를 시간순으로 표시하고 필터링/검색 기능을 제공합니다.
 * 최대 로그 항목 수(MAX_LOG_ENTRIES)를 초과하면 오래된 항목을 자동 삭제합니다.
 */
class AFX_EXT_CLASS CLogOutputPane : public CWnd
{
    DECLARE_DYNAMIC(CLogOutputPane)

public:
    CLogOutputPane();
    virtual ~CLogOutputPane();

    // ========================================================================
    // 갱신 메서드 (UI 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 로그 항목 추가
     *
     * @param threadId  스레드 ID
     * @param level     로그 레벨
     * @param source    로그 발생 소스
     * @param message   로그 메시지
     * @param timestamp 발생 시간
     */
    void addLogEntry(uint32_t threadId, LogLevel level, const std::string& source,
        const std::string& message,
        const std::chrono::system_clock::time_point& timestamp);

    /**
     * @brief 모든 로그 항목을 삭제
     */
    void clearLog();

    // ========================================================================
    // 필터링
    // ========================================================================

    /**
     * @brief 최소 로그 레벨 필터 설정
     * @param minLevel 이 레벨 이상의 로그만 표시
     */
    void setLevelFilter(LogLevel minLevel);

    /**
     * @brief 스레드 필터 설정
     * @param threadId 이 스레드의 로그만 표시 (0 = 모든 스레드)
     */
    void setThreadFilter(uint32_t threadId);

    /**
     * @brief 키워드 검색 설정
     * @param keyword 검색 키워드 (빈 문자열 = 필터 해제)
     */
    void setKeywordFilter(const std::string& keyword);

    /**
     * @brief 자동 스크롤 활성화/비활성화
     * @param enabled true = 자동 스크롤 활성
     */
    void setAutoScroll(bool enabled);

    /**
     * @brief 로그를 파일로 내보내기
     * @param filePath 저장할 파일 경로
     * @return true = 성공
     */
    bool exportToFile(const std::string& filePath) const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void initializeListColumns();
    void rebuildFilteredList();
    bool passesFilter(const LogEntry& entry) const;

    CListCtrl m_listCtrl;              ///< 로그 리스트 컨트롤

    std::deque<LogEntry> m_allEntries;  ///< 전체 로그 항목 (최대 MAX_LOG_ENTRIES)
    std::vector<size_t> m_filteredIndices; ///< 필터를 통과한 항목의 m_allEntries 인덱스

    // 필터 상태
    LogLevel m_minLevelFilter;          ///< 최소 표시 레벨 (기본: Trace)
    uint32_t m_threadFilter;            ///< 스레드 필터 (0 = 모든 스레드)
    std::string m_keywordFilter;        ///< 키워드 필터
    bool m_autoScroll;                  ///< 자동 스크롤 활성 여부

    /// 최대 로그 항목 수. 초과 시 오래된 항목 자동 삭제
    static constexpr int MAX_LOG_ENTRIES = 10000;
};

} // namespace YggdraSeq
