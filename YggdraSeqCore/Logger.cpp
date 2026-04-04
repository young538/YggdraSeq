/**
 * @file Logger.cpp
 * @brief Logger 클래스 구현
 *
 * spdlog를 초기화하고, 파일/콘솔 동기 로깅을 수행합니다.
 * 로그 기록 시 Observer에도 메시지를 전달합니다.
 */

#include "pch.h"
#include "Logger.h"
#include "ISequenceObserver.h"

// spdlog 헤더 (구현 파일에서만 포함하여 헤더 의존성 최소화)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>
#include <cstdlib>

namespace YggdraSeq
{

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

Logger::Logger(const std::string& loggerName)
    : m_loggerName(loggerName)
    , m_level(LogLevel::Info)
    , m_initialized(false)
{
    // 로그 파일 경로 결정 (환경변수 우선, 없으면 기본 경로)
    std::string logPath = Constants::DEFAULT_LOG_PATH;
    const char* envPath = std::getenv("YGGDRASEQ_LOG_PATH");
    if (envPath != nullptr && envPath[0] != '\0')
    {
        logPath = envPath;
    }

    // 로그 레벨 결정 (환경변수 우선)
    const char* envLevel = std::getenv("YGGDRASEQ_LOG_LEVEL");
    if (envLevel != nullptr)
    {
        std::string levelStr(envLevel);
        if (levelStr == "trace")    m_level = LogLevel::Trace;
        else if (levelStr == "debug") m_level = LogLevel::Debug;
        else if (levelStr == "info")  m_level = LogLevel::Info;
        else if (levelStr == "warn")  m_level = LogLevel::Warn;
        else if (levelStr == "error") m_level = LogLevel::Error;
        else if (levelStr == "critical") m_level = LogLevel::Critical;
    }

    // 로그 디렉토리 생성 (존재하지 않으면)
    std::error_code ec;
    std::filesystem::create_directories(logPath, ec);

    // spdlog 싱크 설정 (콘솔 + 파일 동기 모드)
    try
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            logPath + loggerName + ".log", true);

        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };

        m_spdLogger = std::make_shared<spdlog::logger>(loggerName, sinks.begin(), sinks.end());

        // 로그 포맷: [시간] [스레드ID] [레벨] [소스] 메시지
        m_spdLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [T-%t] [%l] %v");

        // spdlog 레벨 설정
        switch (m_level)
        {
        case LogLevel::Trace:    m_spdLogger->set_level(spdlog::level::trace); break;
        case LogLevel::Debug:    m_spdLogger->set_level(spdlog::level::debug); break;
        case LogLevel::Info:     m_spdLogger->set_level(spdlog::level::info); break;
        case LogLevel::Warn:     m_spdLogger->set_level(spdlog::level::warn); break;
        case LogLevel::Error:    m_spdLogger->set_level(spdlog::level::err); break;
        case LogLevel::Critical: m_spdLogger->set_level(spdlog::level::critical); break;
        }

        // 즉시 플러시 (동기 모드 - 로그 순서 보장)
        m_spdLogger->flush_on(spdlog::level::trace);

        // spdlog 전역 레지스트리에 등록
        spdlog::register_logger(m_spdLogger);

        m_initialized = true;
    }
    catch (...)
    {
        // spdlog 초기화 실패 시 nullptr 유지
        m_spdLogger = nullptr;
        m_initialized = false;
    }
}

Logger::~Logger()
{
    if (m_spdLogger)
    {
        m_spdLogger->flush();
        spdlog::drop(m_loggerName);
    }
}

// ============================================================================
// 로그 레벨 설정
// ============================================================================

void Logger::setLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;

    if (m_spdLogger)
    {
        switch (level)
        {
        case LogLevel::Trace:    m_spdLogger->set_level(spdlog::level::trace); break;
        case LogLevel::Debug:    m_spdLogger->set_level(spdlog::level::debug); break;
        case LogLevel::Info:     m_spdLogger->set_level(spdlog::level::info); break;
        case LogLevel::Warn:     m_spdLogger->set_level(spdlog::level::warn); break;
        case LogLevel::Error:    m_spdLogger->set_level(spdlog::level::err); break;
        case LogLevel::Critical: m_spdLogger->set_level(spdlog::level::critical); break;
        }
    }
}

LogLevel Logger::getLevel() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

// ============================================================================
// 로그 기록 메서드
// ============================================================================

void Logger::trace(const std::string& source, const std::string& message)
{
    log(LogLevel::Trace, source, message);
}

void Logger::debug(const std::string& source, const std::string& message)
{
    log(LogLevel::Debug, source, message);
}

void Logger::info(const std::string& source, const std::string& message)
{
    log(LogLevel::Info, source, message);
}

void Logger::warn(const std::string& source, const std::string& message)
{
    log(LogLevel::Warn, source, message);
}

void Logger::error(const std::string& source, const std::string& message)
{
    log(LogLevel::Error, source, message);
}

void Logger::critical(const std::string& source, const std::string& message)
{
    log(LogLevel::Critical, source, message);
}

// ============================================================================
// Observer 연동
// ============================================================================

void Logger::setObservers(const std::vector<ISequenceObserver*>& observers)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers = observers;
}

// ============================================================================
// 초기화 상태
// ============================================================================

bool Logger::isInitialized() const
{
    return m_initialized;
}

// ============================================================================
// private 구현
// ============================================================================

void Logger::log(LogLevel level, const std::string& source, const std::string& message)
{
    // spdlog에 기록 (포맷: [소스] 메시지)
    std::string formattedMsg = "[" + source + "] " + message;

    if (m_spdLogger)
    {
        switch (level)
        {
        case LogLevel::Trace:    m_spdLogger->trace(formattedMsg); break;
        case LogLevel::Debug:    m_spdLogger->debug(formattedMsg); break;
        case LogLevel::Info:     m_spdLogger->info(formattedMsg); break;
        case LogLevel::Warn:     m_spdLogger->warn(formattedMsg); break;
        case LogLevel::Error:    m_spdLogger->error(formattedMsg); break;
        case LogLevel::Critical: m_spdLogger->critical(formattedMsg); break;
        }
    }

    // Observer에 로그 메시지 전달
    notifyObservers(level, source, message);
}

void Logger::notifyObservers(LogLevel level, const std::string& source, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t threadId = static_cast<uint32_t>(::GetCurrentThreadId());

    for (auto* observer : m_observers)
    {
        if (observer != nullptr)
        {
            observer->onLogMessage(threadId, level, source, message);
        }
    }
}

} // namespace YggdraSeq
