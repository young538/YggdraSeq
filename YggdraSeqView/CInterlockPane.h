/**
 * @file CInterlockPane.h
 * @brief 인터락 상태 도킹 패널
 *
 * 전역 인터락의 상태를 신호등(초록/빨강/노랑) 형태로 표시합니다.
 * CDockablePane에서 파생하며, 내부에 CListCtrl을 포함합니다.
 *
 * 표시 항목:
 *   - 인터락 이름
 *   - 상태: 정상(초록) / 위반(빨강) / 경고(노랑)
 *   - 심각도: Warning / Critical / EMO
 *   - 현재 값
 *   - 임계 값
 *   - 마지막 체크 시간
 *   - 체크 스레드 ID
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
 * @class CInterlockPane
 * @brief 인터락 신호등 도킹 패널 (CDockablePane 파생)
 *
 * 인터락 체크 결과를 수신하여 행 배경색으로 상태를 표시합니다.
 * - 정상: 초록 배경
 * - 경고 (Warning 위반): 노랑 배경
 * - 위반 (Critical/EMO): 빨강 배경
 */
class AFX_EXT_CLASS CInterlockPane : public CDockablePane
{
    DECLARE_DYNAMIC(CInterlockPane)

public:
    CInterlockPane();
    virtual ~CInterlockPane();

    // ========================================================================
    // 갱신 메서드 (UI 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 인터락 체크 결과 갱신 (정상 포함)
     *
     * @param threadId      체크 스레드 ID
     * @param interlockName 인터락 이름
     * @param currentValue  현재 값
     * @param passed        체크 통과 여부
     * @param timestamp     체크 시간
     */
    void updateInterlockStatus(uint32_t threadId, const std::string& interlockName,
        const std::string& currentValue, bool passed,
        const std::chrono::system_clock::time_point& timestamp);

    /**
     * @brief 인터락 위반 시 상세 정보 갱신
     *
     * @param threadId      위반 스레드 ID
     * @param interlockName 인터락 이름
     * @param severity      심각도
     * @param currentValue  현재 값
     * @param thresholdValue 임계 값
     * @param timestamp     위반 시간
     */
    void updateInterlockViolation(uint32_t threadId, const std::string& interlockName,
        InterlockSeverity severity, const std::string& currentValue,
        const std::string& thresholdValue,
        const std::chrono::system_clock::time_point& timestamp);

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void initializeListColumns();
    int findOrCreateRow(const std::string& interlockName);

    CListCtrl m_listCtrl;  ///< 인터락 정보 리스트 컨트롤

    /**
     * @struct InterlockInfo
     * @brief 인터락별 상태 추적 구조체
     */
    struct InterlockInfo
    {
        bool passed = true;                 ///< 마지막 체크 결과
        InterlockSeverity severity = InterlockSeverity::Warning;  ///< 심각도
        std::string currentValue;           ///< 현재 값
        std::string thresholdValue;         ///< 임계 값
        int listIndex = -1;                 ///< 리스트 행 인덱스
    };

    std::map<std::string, InterlockInfo> m_interlockInfoMap;  ///< 인터락별 정보 맵
};

} // namespace YggdraSeq
