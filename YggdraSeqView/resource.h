/**
 * @file resource.h
 * @brief YggdraSeqView DLL 리소스 ID 정의
 *
 * 도킹 패널 ID, 커스텀 윈도우 메시지, 리소스 ID를 중앙 관리합니다.
 * View DLL 내부에서 사용하는 모든 ID는 이 파일에서 정의합니다.
 *
 * ID 대역:
 *   - 도킹 패널 ID:    20000 ~ 20099
 *   - 커스텀 메시지:    WM_USER + 1000 ~ WM_USER + 1099
 *   - 타이머 ID:       30000 ~ 30099
 *   - 컨트롤 ID:       40000 ~ 40199
 */

#pragma once

// ============================================================================
// 도킹 패널 ID (CDockablePane 식별용)
// ============================================================================

/** @brief 상태 목록 패널 도킹 ID */
#define IDR_PANE_STATE_LIST         20001

/** @brief 인터락 패널 도킹 ID */
#define IDR_PANE_INTERLOCK          20002

/** @brief 로그 출력 패널 도킹 ID */
#define IDR_PANE_LOG_OUTPUT         20003

/** @brief 스레드 모니터 패널 도킹 ID */
#define IDR_PANE_THREAD_MONITOR     20004

/** @brief 시퀀스 디버그 패널 도킹 ID */
#define IDR_PANE_SEQUENCE_DEBUG     20005

/** @brief 전이 이력 패널 도킹 ID */
#define IDR_PANE_TRANSITION_LOG     20006

// ============================================================================
// 커스텀 윈도우 메시지 (PostMessage로 UI 스레드 갱신용)
// ============================================================================

/** @brief 상태 전이 통지 메시지 (wParam: 전이 데이터 인덱스) */
#define WM_SEQ_STATE_CHANGED        (WM_USER + 1001)

/** @brief 엔진 상태 변경 통지 메시지 (wParam: EngineStatus) */
#define WM_SEQ_ENGINE_STATUS        (WM_USER + 1002)

/** @brief 인터락 위반 통지 메시지 (wParam: 위반 데이터 인덱스) */
#define WM_SEQ_INTERLOCK_VIOLATION  (WM_USER + 1003)

/** @brief 인터락 체크 완료 통지 메시지 (wParam: 체크 데이터 인덱스) */
#define WM_SEQ_INTERLOCK_CHECKED    (WM_USER + 1004)

/** @brief 로그 메시지 통지 메시지 (wParam: 로그 데이터 인덱스) */
#define WM_SEQ_LOG_MESSAGE          (WM_USER + 1005)

/** @brief 에러 통지 메시지 (wParam: 에러 데이터 인덱스) */
#define WM_SEQ_ERROR                (WM_USER + 1006)

/** @brief 스레드 상태 변경 통지 메시지 (wParam: 스레드 데이터 인덱스) */
#define WM_SEQ_THREAD_STATE         (WM_USER + 1007)

/** @brief 디버그 브레이크 통지 메시지 (wParam: 디버그 데이터 인덱스) */
#define WM_SEQ_DEBUG_BREAK          (WM_USER + 1008)

/** @brief 전이 평가 통지 메시지 (wParam: 전이 평가 데이터 인덱스) */
#define WM_SEQ_TRANSITION_EVAL      (WM_USER + 1009)

// ============================================================================
// 타이머 ID
// ============================================================================

/** @brief 로그 패널 자동 스크롤 타이머 */
#define IDT_LOG_AUTO_SCROLL         30001

/** @brief 스레드 모니터 갱신 타이머 */
#define IDT_THREAD_MONITOR_REFRESH  30002

/** @brief 디버그 패널 변수 워치 갱신 타이머 */
#define IDT_DEBUG_WATCH_REFRESH     30003

// ============================================================================
// 컨트롤 ID (자식 윈도우 컨트롤용)
// ============================================================================

/** @brief 상태 목록 CListCtrl */
#define IDC_STATE_LIST              40001

/** @brief 인터락 CListCtrl */
#define IDC_INTERLOCK_LIST          40002

/** @brief 로그 CListCtrl */
#define IDC_LOG_LIST                40003

/** @brief 로그 필터 콤보박스 (스레드) */
#define IDC_LOG_FILTER_THREAD       40004

/** @brief 로그 필터 콤보박스 (레벨) */
#define IDC_LOG_FILTER_LEVEL        40005

/** @brief 로그 검색 에디트 */
#define IDC_LOG_SEARCH_EDIT         40006

/** @brief 로그 자동 스크롤 체크박스 */
#define IDC_LOG_AUTO_SCROLL         40007

/** @brief 스레드 모니터 CListCtrl */
#define IDC_THREAD_LIST             40008

/** @brief 디버그 상태 CListCtrl (전이 조건 모니터) */
#define IDC_DEBUG_TRANSITION_LIST   40009

/** @brief 디버그 변수 워치 CListCtrl */
#define IDC_DEBUG_WATCH_LIST        40010

/** @brief 디버그 브레이크포인트 CListCtrl */
#define IDC_DEBUG_BREAKPOINT_LIST   40011

/** @brief 디버그 콜스택 CListCtrl */
#define IDC_DEBUG_CALLSTACK_LIST    40012

/** @brief 디버그 Step Next 버튼 */
#define IDC_DEBUG_STEP_NEXT         40013

/** @brief 전이 이력 CListCtrl */
#define IDC_TRANSITION_LIST         40014

/** @brief 전이 필터 콤보박스 (스레드) */
#define IDC_TRANSITION_FILTER_THREAD 40015

/** @brief 전이 필터 체크박스 (실패만) */
#define IDC_TRANSITION_FILTER_FAIL  40016

/** @brief 전이 CSV 내보내기 버튼 */
#define IDC_TRANSITION_EXPORT_CSV   40017

/** @brief 로그 내보내기 버튼 */
#define IDC_LOG_EXPORT              40018
