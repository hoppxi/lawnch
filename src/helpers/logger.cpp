#include "logger.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>

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

  void configure(const std::string &path, bool verbose, bool print_logs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_verbose = verbose;
    m_print_logs = print_logs;

    if (m_print_logs) {
      if (m_file.is_open()) {
        m_file.close();
      }
      return;
    }

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
    if (!m_verbose.load()) {
      if (level == LogLevel::DEBUG || level == LogLevel::INFO) {
        return;
      }
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::lock_guard<std::mutex> lock(m_mutex);
    auto tm_now = *std::localtime(&time_t_now);

    std::ostream &out = get_output_stream();

    std::string reset = "";
    std::string color_date = "";
    std::string color_level = "";
    std::string color_name = "";
    std::string color_msg = "";
    std::string final_msg = std::string(message);

    if (m_print_logs) {
      reset = "\033[0m";
      color_date = "\033[90m"; // Bright Gray
      color_name = "\033[35m"; // Purple
      color_msg = "\033[97m";  // Bright White

      switch (level) {
      case LogLevel::DEBUG:
        color_level = "\033[36m"; // Cyan
        break;
      case LogLevel::INFO:
        color_level = "\033[32m"; // Green
        break;
      case LogLevel::WARNING:
        color_level = "\033[33m"; // Yellow
        break;
      case LogLevel::ERROR:
        color_level = "\033[31m"; // Red
        break;
      case LogLevel::CRITICAL:
        color_level = "\033[1;31m"; // Bold Red
        break;
      }

      // Syntax highlight message: quotes (cyan) and numbers (yellow)
      std::regex quote_re("(\"[^\"]*\"|'[^']*')");
      std::string color_quote = "\033[96m"; // Bright Cyan
      final_msg = std::regex_replace(final_msg, quote_re,
                                     color_quote + "$1" + color_msg);

      std::regex num_re("\\b(\\d+\\.?\\d*)\\b");
      std::string color_num = "\033[93m"; // Bright Yellow
      final_msg =
          std::regex_replace(final_msg, num_re, color_num + "$1" + color_msg);
    }

    if (m_print_logs) {
      out << color_date << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")
          << "] " << reset;
      out << color_level << "[" << std::right << std::setw(WIDTH_LEVEL)
          << levelToString(level) << "] " << reset;
      out << color_name << "[" << std::right << std::setw(WIDTH_NAME)
          << logger_name << "] " << reset;
      out << color_msg << final_msg << reset << "\n";
      out.flush();
    } else {
      out << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] ";
      out << "[" << std::right << std::setw(WIDTH_LEVEL) << levelToString(level)
          << "] ";
      out << "[" << std::right << std::setw(WIDTH_NAME) << logger_name << "] ";
      out << final_msg << "\n";
    }
  }

private:
  LogEngine() : m_verbose(false), m_print_logs(false) {}

  ~LogEngine() {
    if (m_file.is_open()) {
      m_file.close();
    }
  }

  std::ostream &get_output_stream() {
    if (m_print_logs) {
      return std::cout;
    }
    if (!m_file.is_open()) {
      return std::cout;
    }
    return m_file;
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
  std::atomic<bool> m_verbose;
  std::atomic<bool> m_print_logs;
};

void init(const std::string &file_path, bool verbose, bool print_logs) {
  LogEngine::getInstance().configure(file_path, verbose, print_logs);
}

void log(std::string_view logger_name, LogLevel level,
         std::string_view message) {
  LogEngine::getInstance().write(logger_name, level, message);
}

} // namespace Lawnch::Logger
