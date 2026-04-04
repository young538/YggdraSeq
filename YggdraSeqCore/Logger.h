/**
 * @file Logger.h
 * @brief spdlog 래퍼 클래스
 *
 * spdlog를 래핑하여 프레임워크 전체에서 일관된 로깅 인터페이스를 제공합니다.
 * 동기 모드를 기본으로 사용하며, 차후 하이브리드(동기+비동기) 전환이
 * 가능하도록 추상화되어 있습니다.
 *
 * 주요 기능:
 * - spdlog 기반 파일/콘솔 로깅
 * - ISequenceObserver에 로그 메시지 전달
 * - 로그 레벨별 필터링
 * - 스레드 ID 자동 포함
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

// spdlog 전방 선언 (헤더 의존성 최소화)
namespace spdlog
{
    class logger;
}

namespace YggdraSeq
{

// 전방 선언
class ISequenceObserver;

/**
 * @class Logger
 * @brief spdlog를 래핑하는 로거 클래스
 *
 * 프레임워크 전체에서 사용하는 로깅 유틸리티입니다.
 * 동기 모드로 로그 순서를 보장하며, Observer에도 로그를 전달하여
 * View의 CLogOutputPane에서 실시간 표시할 수 있습니다.
 *
 * 사용 예:
 * @code
 * auto logger = std::make_shared<Logger>("SequenceEngine");
 * logger->info("Engine", "Sequence started");
 * logger->error("Interlock", "EMO triggered: {}", interlockName);
 * @endcode
 *
 * @note 스레드 안전합니다. 동시에 여러 스레드에서 로그를 기록해도 안전합니다.
 */
class AFX_EXT_CLASS Logger
{
public:
    /**
     * @brief Logger 생성자
     *
     * spdlog 로거를 초기화하고 파일/콘솔 싱크를 설정합니다.
     * 로그 파일은 YGGDRASEQ_LOG_PATH 환경변수 또는 기본 경로(./logs/)에 생성됩니다.
     *
     * @param loggerName 로거 이름 (spdlog 레지스트리에 등록되는 이름)
     */
    explicit Logger(const std::string& loggerName);

    /**
     * @brief 소멸자
     */
    ~Logger();

    // ========================================================================
    // 로그 레벨 설정
    // ========================================================================

    /**
     * @brief 로그 레벨 설정
     *
     * 설정된 레벨 이상의 로그만 기록됩니다.
     *
     * @param level 최소 로그 레벨
     */
    void setLevel(LogLevel level);

    /**
     * @brief 현재 로그 레벨 반환
     * @return 현재 설정된 최소 로그 레벨
     */
    LogLevel getLevel() const;

    // ========================================================================
    // 로그 기록 메서드
    // ========================================================================

    /**
     * @brief TRACE 레벨 로그 기록
     * @param source  로그 발생 소스 (클래스명 또는 모듈명)
     * @param message 로그 메시지
     */
    void trace(const std::string& source, const std::string& message);

    /**
     * @brief DEBUG 레벨 로그 기록
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void debug(const std::string& source, const std::string& message);

    /**
     * @brief INFO 레벨 로그 기록
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void info(const std::string& source, const std::string& message);

    /**
     * @brief WARN 레벨 로그 기록
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void warn(const std::string& source, const std::string& message);

    /**
     * @brief ERROR 레벨 로그 기록
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void error(const std::string& source, const std::string& message);

    /**
     * @brief CRITICAL 레벨 로그 기록
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void critical(const std::string& source, const std::string& message);

    // ========================================================================
    // Observer 연동
    // ========================================================================

    /**
     * @brief Observer 목록을 설정 (로그 메시지 전달용)
     *
     * 로그가 기록될 때 등록된 Observer들의 onLogMessage()를 호출합니다.
     * SequenceEngine에서 Observer 목록이 변경될 때마다 이 메서드를 호출합니다.
     *
     * @param observers Observer 포인터 목록 (소유권 없음, 관찰만)
     * @note Observer의 수명은 이 Logger보다 길어야 합니다.
     */
    void setObservers(const std::vector<ISequenceObserver*>& observers);

    // ========================================================================
    // 초기화 상태
    // ========================================================================

    /**
     * @brief 로거가 정상적으로 초기화되었는지 확인
     * @return true = 정상 초기화됨
     */
    bool isInitialized() const;

private:
    /**
     * @brief 공통 로그 기록 로직
     *
     * spdlog에 기록하고, 설정된 Observer들에게 onLogMessage()로 전달합니다.
     *
     * @param level   로그 레벨
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void log(LogLevel level, const std::string& source, const std::string& message);

    /**
     * @brief Observer들에게 로그 메시지를 전달
     *
     * @param level   로그 레벨
     * @param source  로그 발생 소스
     * @param message 로그 메시지
     */
    void notifyObservers(LogLevel level, const std::string& source, const std::string& message);

    std::shared_ptr<spdlog::logger> m_spdLogger;    ///< spdlog 로거 인스턴스
    std::string m_loggerName;                        ///< 로거 이름
    LogLevel m_level;                                ///< 현재 로그 레벨
    bool m_initialized;                              ///< 초기화 상태

    std::vector<ISequenceObserver*> m_observers;     ///< Observer 목록 (관찰용)
    mutable std::mutex m_mutex;                      ///< 스레드 동기화용 뮤텍스
};

} // namespace YggdraSeq
