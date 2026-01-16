#include "logger.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace Lawnch::Logger {

static constexpr int WIDTH_LEVEL = 8;
static constexpr int WIDTH_NAME = 15;

class LogEngine {
public:
  static LogEngine &getInstance() {
    static LogEngine instance;
    return instance;
  }

  LogEngine(const LogEngine &) = delete;
  LogEngine &operator=(const LogEngine &) = delete;

  void configure(const std::string &path, LogVerbosity verbosity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_verbosity = verbosity;

    if (m_file.is_open()) {
      m_file.close();
    }
    m_file.open(path, std::ios::out | std::ios::app);

    if (!m_file.is_open()) {
      std::cerr << "[Logger] Failed to open log file: " << path << std::endl;
    }
  }

  void write(std::string_view logger_name, LogLevel level,
             std::string_view message) {
    int level_val = 0;
    switch (level) {
    case LogLevel::CRITICAL:
    case LogLevel::ERROR:
      level_val = 1;
      break;
    case LogLevel::WARNING:
      level_val = 2;
      break;
    case LogLevel::INFO:
      level_val = 3;
      break;
    case LogLevel::DEBUG:
      level_val = 4;
      break;
    }

    if (level_val > static_cast<int>(m_verbosity.load())) {
      return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::lock_guard<std::mutex> lock(m_mutex);
    auto tm_now = *std::localtime(&time_t_now);

    if (!m_file.is_open()) {
      std::cout << "[LOG_FAILED] " << message << "\n";
      return;
    }

    m_file << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] ";
    m_file << "[" << std::right << std::setw(WIDTH_LEVEL)
           << levelToString(level) << "] ";
    m_file << "[" << std::right << std::setw(WIDTH_NAME) << logger_name << "] ";
    m_file << message << "\n";
  }

private:
  LogEngine() : m_verbosity(LogVerbosity::LEVEL_3) {}

  ~LogEngine() {
    if (m_file.is_open()) {
      m_file.close();
    }
  }

  std::string_view levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
    }
  }

  std::ofstream m_file;
  std::mutex m_mutex;
  std::atomic<LogVerbosity> m_verbosity;
};

void init(const std::string &file_path, LogVerbosity verbosity) {
  LogEngine::getInstance().configure(file_path, verbosity);
}

void log(std::string_view logger_name, LogLevel level,
         std::string_view message) {
  LogEngine::getInstance().write(logger_name, level, message);
}

} // namespace Lawnch::Logger
