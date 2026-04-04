/**
 * @file resource.h
 * @brief YggdraSeqApp 리소스 ID 정의
 *
 * HMI 테스트 앱에서 사용하는 모든 리소스 ID를 중앙 관리합니다.
 *
 * ID 대역:
 *   - 메인 프레임:     100 ~ 199
 *   - 툴바/메뉴:       200 ~ 299
 *   - Activity Bar:    300 ~ 399
 *   - 메인 뷰:         400 ~ 499
 *   - 커스텀 메시지:    WM_USER + 2000 ~ WM_USER + 2099
 *   - 타이머:           500 ~ 599
 */

#pragma once

// ============================================================================
// 메인 프레임
// ============================================================================

#define IDR_MAINFRAME               100
#define IDR_MAIN_TOOLBAR            101

// ============================================================================
// 시퀀스 제어 명령 ID
// ============================================================================

#define ID_SEQ_RUN                  201
#define ID_SEQ_PAUSE                202
#define ID_SEQ_STOP                 203
#define ID_SEQ_EMO                  204
#define ID_SEQ_RECIPE_COMBO         205
#define ID_SEQ_MODE_AUTO            206
#define ID_SEQ_MODE_MANUAL          207
#define ID_SEQ_MODE_DEBUG           208
#define ID_SEQ_MODE_COMBO           209

// ============================================================================
// Activity Bar 아이콘 ID
// ============================================================================

#define ID_ACTIVITY_PROCESS         301
#define ID_ACTIVITY_DEBUG           302
#define ID_ACTIVITY_SETTINGS        303

// ============================================================================
// 메인 뷰 컨트롤 ID
// ============================================================================

#define IDC_PROCESS_MIMIC           401
#define IDC_SENSOR_DATA_LIST        402

// ============================================================================
// 도킹 패널 ID (App 고유)
// ============================================================================

#define IDR_PANE_PROCESS_MIMIC      410
#define IDR_PANE_SENSOR_DATA        411

// ============================================================================
// 커스텀 메시지 (App 전용)
// ============================================================================

/** @brief SimulatedHardware 값 변경 통지 */
#define WM_APP_HARDWARE_UPDATED     (WM_USER + 2001)

/** @brief 시퀀스 상태 갱신 통지 (미믹 다이어그램용) */
#define WM_APP_SEQ_STATE_UPDATED    (WM_USER + 2002)

/** @brief 인터락 위반 통지 (미믹 다이어그램용) */
#define WM_APP_INTERLOCK_ALERT      (WM_USER + 2003)

// ============================================================================
// 타이머 ID
// ============================================================================

/** @brief 미믹 다이어그램 애니메이션 타이머 (50ms) */
#define IDT_MIMIC_ANIMATION         501

/** @brief 센서 데이터 갱신 타이머 (100ms) */
#define IDT_SENSOR_REFRESH          502

/** @brief 상태 바 갱신 타이머 (500ms) */
#define IDT_STATUSBAR_REFRESH       503

// ============================================================================
// 상태 바 패인 ID
// ============================================================================

#define ID_STATUSBAR_MODE           601
#define ID_STATUSBAR_INSPECT        602
#define ID_STATUSBAR_HANDLER        603
#define ID_STATUSBAR_THREADS        604
#define ID_STATUSBAR_ERRORS         605
#define ID_STATUSBAR_INTERLOCKS     606
