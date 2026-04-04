/**
 * @file CActivityBar.h
 * @brief VS Code Activity Bar 스타일 좌측 아이콘 사이드바
 *
 * 좌측에 고정된 아이콘 버튼 바를 구현합니다.
 * CWnd에서 파생하며, 아이콘 클릭 시 메인 영역의 탭 그룹을 전환합니다.
 *
 * 아이콘:
 *   - 공정 (Process): 미믹 다이어그램 + 센서 데이터 표시
 *   - 디버그 (Debug): 디버그 패널 표시
 *   - 설정 (Settings): 설정 패널 표시
 *
 * 스타일:
 *   - 다크 배경 (#333333)
 *   - 아이콘 색상 (#cccccc)
 *   - 선택 시 (#ffffff) + 좌측 테두리 강조
 *   - 너비: 48px 고정
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include <functional>

namespace YggdraSeq
{

/**
 * @enum ActivityMode
 * @brief Activity Bar 모드
 */
enum class ActivityMode
{
    Process,    ///< 공정 모드 (미믹 + 센서)
    Debug,      ///< 디버그 모드
    Settings    ///< 설정 모드
};

/**
 * @class CActivityBar
 * @brief VS Code Activity Bar 스타일 좌측 아이콘 사이드바 (CWnd 파생)
 *
 * 세로로 고정된 아이콘 버튼 바입니다.
 * Owner Draw로 아이콘과 선택 표시를 직접 그립니다.
 */
class CActivityBar : public CWnd
{
    DECLARE_DYNAMIC(CActivityBar)

public:
    CActivityBar();
    virtual ~CActivityBar();

    /**
     * @brief Activity Bar 생성
     *
     * @param pParentWnd 부모 윈도우
     * @param rect       초기 위치 (일반적으로 좌측에 고정)
     * @return TRUE = 성공
     */
    BOOL CreateBar(CWnd* pParentWnd, const CRect& rect);

    /**
     * @brief 현재 선택된 모드 반환
     * @return 현재 ActivityMode
     */
    ActivityMode getCurrentMode() const { return m_currentMode; }

    /**
     * @brief 모드 변경 시 호출될 콜백 설정
     * @param callback 콜백 함수 (ActivityMode 인자)
     */
    void setModeChangeCallback(std::function<void(ActivityMode)> callback);

    /// Activity Bar 고정 너비 (48px)
    static constexpr int BAR_WIDTH = 48;

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    /**
     * @brief 아이콘 영역을 계산
     * @param index 아이콘 인덱스 (0=Process, 1=Debug, 2=Settings)
     * @return 아이콘의 CRect 영역
     */
    CRect getIconRect(int index) const;

    /**
     * @brief 아이콘 심볼을 그리기
     * @param pDC    디바이스 컨텍스트
     * @param rect   그리기 영역
     * @param index  아이콘 인덱스
     * @param selected 선택 상태 여부
     */
    void drawIcon(CDC* pDC, const CRect& rect, int index, bool selected);

    ActivityMode m_currentMode;                     ///< 현재 선택 모드
    std::function<void(ActivityMode)> m_callback;   ///< 모드 변경 콜백

    static constexpr int ICON_SIZE = 28;            ///< 아이콘 크기
    static constexpr int ICON_SPACING = 8;          ///< 아이콘 간격
    static constexpr int ICON_COUNT = 3;            ///< 아이콘 수
};

} // namespace YggdraSeq
