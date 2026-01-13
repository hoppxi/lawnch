#include "logger.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace Lawnch::Logger {

class LogEngine {
public:
  static LogEngine &getInstance() {
    static LogEngine instance;
    return instance;
  }

  LogEngine(const LogEngine &) = delete;
  LogEngine &operator=(const LogEngine &) = delete;

  void setLogFile(const std::string &path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
      m_file.close();
    }
    m_file.open(path, std::ios::out | std::ios::app);

    if (!m_file.is_open()) {
      std::cerr << "[Logger] Failed to open log file: " << path << std::endl;
    }
  }

  void write(std::string_view logger_name, Lawnch::Logger::LogLevel level,
             std::string_view message) {

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto tm_now = *std::localtime(&time_t_now);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_file.is_open()) {
      // fallback to console if file isn't open
      std::cout << "[LOG_FAILED] " << message << "\n";
      return;
    }

    m_file << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] ";
    m_file << "[" << logger_name << "] ";
    m_file << "[" << levelToString(level) << "] ";
    m_file << message << "\n";
  }

private:
  LogEngine() = default;

  ~LogEngine() {
    if (m_file.is_open()) {
      m_file.close();
    }
  }

  std::string_view levelToString(Lawnch::Logger::LogLevel level) {
    using Lawnch::Logger::LogLevel;
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
};

void init(const std::string &file_path) {
  LogEngine::getInstance().setLogFile(file_path);
}

void log(std::string_view logger_name, LogLevel level,
         std::string_view message) {
  LogEngine::getInstance().write(logger_name, level, message);
}

} // namespace Lawnch::Logger
