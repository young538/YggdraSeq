/**
 * @file Types.h
 * @brief YggdraSeq 공통 열거형, 에러 코드, 타입 정의
 *
 * 프레임워크 전체에서 사용하는 공통 타입을 중앙 관리합니다.
 * 모든 열거형과 타입 별칭을 이 파일에서 정의하여 일관성을 유지합니다.
 *
 * @namespace YggdraSeq
 * @note 새로운 에러 코드나 상태 타입을 추가할 때는 반드시 이 파일에 추가하세요.
 */

#pragma once

#include <string>
#include <variant>
#include <chrono>
#include <vector>
#include <functional>

namespace YggdraSeq
{

// ============================================================================
// 시퀀스 엔진 상태
// ============================================================================

/**
 * @enum EngineStatus
 * @brief 시퀀스 엔진의 현재 실행 상태
 *
 * 엔진의 생명주기를 나타냅니다.
 * 상태 전이: Idle -> Running -> (Paused -> Running) -> Complete
 *                          -> Error -> (복구 후 Idle)
 *                          -> Aborted -> Idle
 */
enum class EngineStatus
{
    Idle,       ///< 대기 중. 시퀀스가 로드되었지만 실행되지 않은 상태
    Running,    ///< 실행 중. 상태 머신이 활발히 전이를 수행하는 상태
    Paused,     ///< 일시 정지. 현재 상태의 onExecute() 완료 후 대기 중
    Error,      ///< 에러 발생. 인터락 위반 또는 상태 실행 오류로 정지
    Aborted,    ///< 강제 중단. abort() 호출로 시퀀스가 중단된 상태
    Complete    ///< 완료. 모든 상태 전이가 정상적으로 끝난 상태
};

/**
 * @brief EngineStatus를 문자열로 변환
 * @param status 변환할 엔진 상태
 * @return 상태에 대응하는 문자열 (예: "Running")
 */
inline const char* engineStatusToString(EngineStatus status)
{
    switch (status)
    {
    case EngineStatus::Idle:     return "Idle";
    case EngineStatus::Running:  return "Running";
    case EngineStatus::Paused:   return "Paused";
    case EngineStatus::Error:    return "Error";
    case EngineStatus::Aborted:  return "Aborted";
    case EngineStatus::Complete: return "Complete";
    default:                     return "Unknown";
    }
}

// ============================================================================
// 상태 유형
// ============================================================================

/**
 * @enum StateType
 * @brief IState 구현체의 유형을 분류
 *
 * 시퀀스 내에서 각 상태의 역할을 나타냅니다.
 * 모니터링 뷰에서 상태를 색상/아이콘으로 구분하는 데 활용합니다.
 */
enum class StateType
{
    Init,       ///< 초기화 상태. 장비 원점 복귀, 변수 초기화 등
    Process,    ///< 공정 상태. 실제 작업 수행 (검사, 이송 등)
    Wait,       ///< 대기 상태. 외부 이벤트나 조건 충족을 기다림
    End         ///< 종료 상태. 시퀀스 종료 처리 (정리, 보고 등)
};

/**
 * @brief StateType을 문자열로 변환
 * @param type 변환할 상태 유형
 * @return 유형에 대응하는 문자열 (예: "Process")
 */
inline const char* stateTypeToString(StateType type)
{
    switch (type)
    {
    case StateType::Init:    return "Init";
    case StateType::Process: return "Process";
    case StateType::Wait:    return "Wait";
    case StateType::End:     return "End";
    default:                 return "Unknown";
    }
}

// ============================================================================
// 인터락 심각도
// ============================================================================

/**
 * @enum InterlockSeverity
 * @brief 인터락 위반의 심각도 수준
 *
 * 위반 발생 시 시스템의 대응 방식을 결정합니다.
 * - Warning: 로그 기록만 수행하고 시퀀스는 계속 진행
 * - Critical: 시퀀스를 즉시 일시 정지하고 운전자 개입 대기
 * - EMO: 모든 스레드를 즉시 비상 정지하고 EMO 콜백 호출
 */
enum class InterlockSeverity
{
    Warning,    ///< 경고. 로그 기록, 시퀀스 계속 진행
    Critical,   ///< 위험. 시퀀스 일시 정지, 운전자 개입 필요
    EMO         ///< 비상 정지. 모든 동작 즉시 중단, EMO 콜백 호출
};

/**
 * @brief InterlockSeverity를 문자열로 변환
 * @param severity 변환할 심각도
 * @return 심각도에 대응하는 문자열 (예: "Critical")
 */
inline const char* interlockSeverityToString(InterlockSeverity severity)
{
    switch (severity)
    {
    case InterlockSeverity::Warning:  return "Warning";
    case InterlockSeverity::Critical: return "Critical";
    case InterlockSeverity::EMO:      return "EMO";
    default:                          return "Unknown";
    }
}

// ============================================================================
// 에러 코드
// ============================================================================

/**
 * @enum ErrorCode
 * @brief 프레임워크 전체에서 사용하는 에러 코드
 *
 * 예외(exception) 대신 에러 코드를 반환하여 에러를 처리합니다.
 * 반도체 장비 제어에서는 예외가 위험할 수 있으므로 에러 코드 기반으로 설계합니다.
 *
 * @note 새로운 에러 코드 추가 시 카테고리별 번호 대역을 유지하세요.
 *       0~99: 성공/일반, 100~199: 엔진, 200~299: 상태, 300~399: 인터락, 400~499: 레시피
 */
enum class ErrorCode
{
    // --- 성공 / 일반 (0~99) ---
    Success = 0,                        ///< 성공
    UnknownError = 1,                   ///< 알 수 없는 에러

    // --- 엔진 관련 (100~199) ---
    EngineAlreadyRunning = 100,         ///< 엔진이 이미 실행 중
    EngineNotRunning = 101,             ///< 엔진이 실행 중이 아님
    EngineInErrorState = 102,           ///< 엔진이 에러 상태
    EngineAborted = 103,                ///< 엔진이 중단됨
    EngineNoStatesRegistered = 104,     ///< 등록된 상태가 없음
    EngineNoInitialState = 105,         ///< 초기 상태가 설정되지 않음
    EngineThreadStartFailed = 106,      ///< 워커 스레드 시작 실패

    // --- 상태 관련 (200~299) ---
    StateNotFound = 200,                ///< 지정된 상태를 찾을 수 없음
    StateAlreadyRegistered = 201,       ///< 같은 이름의 상태가 이미 등록됨
    StateEntryFailed = 202,             ///< 상태 진입(onEntry) 실패
    StateExecutionFailed = 203,         ///< 상태 실행(onExecute) 실패
    StateExitFailed = 204,              ///< 상태 퇴장(onExit) 실패
    StateTransitionFailed = 205,        ///< 상태 전이 실패 (조건 불충족)
    StateNoValidTransition = 206,       ///< 유효한 전이가 없음 (데드락 가능)

    // --- 인터락 관련 (300~399) ---
    InterlockViolation = 300,           ///< 인터락 조건 위반
    InterlockEMO = 301,                 ///< EMO 인터락 발동
    InterlockNotFound = 302,            ///< 지정된 인터락을 찾을 수 없음
    InterlockAlreadyRegistered = 303,   ///< 같은 이름의 인터락이 이미 등록됨
    InterlockCheckFailed = 304,         ///< 인터락 체크 함수 실행 중 오류

    // --- 레시피 관련 (400~499) — Phase 2 ---
    RecipeLoadFailed = 400,             ///< 레시피 파일 로드 실패
    RecipeParseError = 401,             ///< 레시피 JSON 파싱 오류
    RecipeValidationFailed = 402,       ///< 레시피 검증 실패 (파라미터 범위 초과 등)
    RecipeStepNotFound = 403,           ///< 레시피 스텝에 해당하는 상태가 없음
    RecipeNotLoaded = 404,              ///< 레시피가 로드되지 않음

    // --- 로거 관련 (500~599) ---
    LoggerInitFailed = 500,             ///< 로거 초기화 실패

    // --- 디버그 관련 (600~699) ---
    DebugBreakpointHit = 600,           ///< 브레이크포인트 도달 (에러가 아님, 정보성)
    DebugStepCompleted = 601            ///< 스텝 실행 완료 (에러가 아님, 정보성)
};

/**
 * @brief ErrorCode를 문자열로 변환
 * @param code 변환할 에러 코드
 * @return 에러 코드에 대응하는 문자열 설명
 */
inline const char* errorCodeToString(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::Success:                    return "Success";
    case ErrorCode::UnknownError:               return "Unknown Error";
    case ErrorCode::EngineAlreadyRunning:        return "Engine Already Running";
    case ErrorCode::EngineNotRunning:            return "Engine Not Running";
    case ErrorCode::EngineInErrorState:          return "Engine In Error State";
    case ErrorCode::EngineAborted:               return "Engine Aborted";
    case ErrorCode::EngineNoStatesRegistered:    return "No States Registered";
    case ErrorCode::EngineNoInitialState:        return "No Initial State";
    case ErrorCode::EngineThreadStartFailed:     return "Thread Start Failed";
    case ErrorCode::StateNotFound:               return "State Not Found";
    case ErrorCode::StateAlreadyRegistered:      return "State Already Registered";
    case ErrorCode::StateEntryFailed:            return "State Entry Failed";
    case ErrorCode::StateExecutionFailed:        return "State Execution Failed";
    case ErrorCode::StateExitFailed:             return "State Exit Failed";
    case ErrorCode::StateTransitionFailed:       return "State Transition Failed";
    case ErrorCode::StateNoValidTransition:      return "No Valid Transition";
    case ErrorCode::InterlockViolation:          return "Interlock Violation";
    case ErrorCode::InterlockEMO:                return "EMO Interlock Triggered";
    case ErrorCode::InterlockNotFound:           return "Interlock Not Found";
    case ErrorCode::InterlockAlreadyRegistered:  return "Interlock Already Registered";
    case ErrorCode::InterlockCheckFailed:        return "Interlock Check Failed";
    case ErrorCode::RecipeLoadFailed:            return "Recipe Load Failed";
    case ErrorCode::RecipeParseError:            return "Recipe Parse Error";
    case ErrorCode::RecipeValidationFailed:      return "Recipe Validation Failed";
    case ErrorCode::RecipeStepNotFound:          return "Recipe Step Not Found";
    case ErrorCode::RecipeNotLoaded:             return "Recipe Not Loaded";
    case ErrorCode::LoggerInitFailed:            return "Logger Init Failed";
    case ErrorCode::DebugBreakpointHit:          return "Debug Breakpoint Hit";
    case ErrorCode::DebugStepCompleted:          return "Debug Step Completed";
    default:                                     return "Unknown Error Code";
    }
}

// ============================================================================
// 로그 레벨
// ============================================================================

/**
 * @enum LogLevel
 * @brief 로그 메시지의 심각도 수준
 *
 * spdlog의 로그 레벨과 1:1 대응됩니다.
 * Observer를 통해 View에 로그 레벨 정보를 전달할 때 사용합니다.
 */
enum class LogLevel
{
    Trace,      ///< 상세 추적 (개발 중 디버깅용)
    Debug,      ///< 디버그 정보 (개발/테스트 시)
    Info,       ///< 일반 정보 (정상 동작 기록)
    Warn,       ///< 경고 (주의 필요하지만 동작은 계속)
    Error,      ///< 에러 (기능 오작동)
    Critical    ///< 치명적 (시스템 정지 수준)
};

/**
 * @brief LogLevel을 문자열로 변환
 * @param level 변환할 로그 레벨
 * @return 로그 레벨에 대응하는 문자열 (예: "INFO")
 */
inline const char* logLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:    return "TRACE";
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO";
    case LogLevel::Warn:     return "WARN";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    default:                 return "UNKNOWN";
    }
}

// ============================================================================
// 스레드 상태
// ============================================================================

/**
 * @enum ThreadState
 * @brief 워커 스레드의 현재 상태
 *
 * CThreadMonitorPane에서 각 스레드의 상태를 표시할 때 사용합니다.
 */
enum class ThreadState
{
    Running,    ///< 활발히 실행 중
    Waiting,    ///< 조건/이벤트 대기 중
    Blocked,    ///< 동기화 객체(mutex 등)에 의해 차단됨
    Idle,       ///< 유휴 상태 (작업 없음)
    Terminated  ///< 종료됨
};

/**
 * @brief ThreadState를 문자열로 변환
 * @param state 변환할 스레드 상태
 * @return 상태에 대응하는 문자열 (예: "Running")
 */
inline const char* threadStateToString(ThreadState state)
{
    switch (state)
    {
    case ThreadState::Running:    return "Running";
    case ThreadState::Waiting:    return "Waiting";
    case ThreadState::Blocked:    return "Blocked";
    case ThreadState::Idle:       return "Idle";
    case ThreadState::Terminated: return "Terminated";
    default:                      return "Unknown";
    }
}

// ============================================================================
// 공통 타입 별칭
// ============================================================================

/**
 * @brief 레시피 파라미터 값으로 사용할 수 있는 variant 타입
 *
 * int, double, std::string, bool 4가지 타입을 지원합니다.
 * RecipeStep의 params에서 사용합니다.
 */
using ParamValue = std::variant<int, double, std::string, bool>;

/**
 * @brief 고해상도 시계 타입 별칭 (밀리초 단위 측정에 적합)
 */
using Clock = std::chrono::steady_clock;

/**
 * @brief 시간 포인트 타입 별칭
 */
using TimePoint = std::chrono::steady_clock::time_point;

/**
 * @brief 시간 간격 타입 별칭 (밀리초)
 */
using Duration = std::chrono::milliseconds;

// ============================================================================
// 인터락 체크 결과 구조체
// ============================================================================

/**
 * @struct InterlockCheckResult
 * @brief 개별 인터락 체크 결과를 담는 구조체
 *
 * Transition 평가 시 각 인터락의 체크 결과를 Observer에 전달할 때 사용합니다.
 * 현재값과 결과를 함께 전달하여 View에서 상세 모니터링이 가능합니다.
 */
struct InterlockCheckResult
{
    std::string interlockName;      ///< 인터락 이름
    bool passed;                    ///< 체크 통과 여부 (true = 안전)
    InterlockSeverity severity;     ///< 인터락 심각도
    std::string description;        ///< 인터락 설명
    std::string currentValue;       ///< 현재 값 (문자열로 변환하여 전달)
    std::string thresholdValue;     ///< 임계 값 (문자열로 변환하여 전달)
};

// ============================================================================
// 상수 정의
// ============================================================================

namespace Constants
{
    /// 기본 엔진 루프 주기 (밀리초). 너무 짧으면 CPU 부하, 너무 길면 반응 지연
    constexpr int DEFAULT_ENGINE_LOOP_INTERVAL_MS = 10;

    /// 인터락 체크 기본 타임아웃 (밀리초)
    constexpr int DEFAULT_INTERLOCK_TIMEOUT_MS = 1000;

    /// 최대 Observer 수. 무한 등록을 방지
    constexpr int MAX_OBSERVER_COUNT = 32;

    /// 최대 상태 수. 무한 등록을 방지
    constexpr int MAX_STATE_COUNT = 256;

    /// 최대 전이 수 (상태당). 복잡도 제한
    constexpr int MAX_TRANSITION_PER_STATE = 32;

    /// 기본 로그 파일 경로
    constexpr const char* DEFAULT_LOG_PATH = "./logs/";

    /// 기본 레시피 파일 경로
    constexpr const char* DEFAULT_RECIPE_PATH = "./recipes/";
}

} // namespace YggdraSeq
