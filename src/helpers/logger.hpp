#pragma once

#include <string>
#include <string_view>

namespace Lawnch::Logger {

enum class LogLevel { CRITICAL, ERROR, WARNING, INFO, DEBUG };

void init(const std::string &file_path, bool verbose = false,
          bool print_logs = false);
void log(std::string_view logger_name, LogLevel level,
         std::string_view message);

} // namespace Lawnch::Logger
