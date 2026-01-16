#pragma once

#include <string>
#include <string_view>

namespace Lawnch::Logger {

enum class LogLevel { CRITICAL, ERROR, WARNING, INFO, DEBUG };

// 1 = error and critical only
// 2 = warnings + above
// 3 = info + above
// 4 = debug + above
enum class LogVerbosity : int {
  LEVEL_1 = 1,
  LEVEL_2 = 2,
  LEVEL_3 = 3,
  LEVEL_4 = 4
};

void init(const std::string &file_path,
          LogVerbosity verbosity = LogVerbosity::LEVEL_3);
void log(std::string_view logger_name, LogLevel level,
         std::string_view message);

} // namespace Lawnch::Logger
