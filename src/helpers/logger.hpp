#pragma once

#include <string>
#include <string_view>

namespace Lawnch::Logger {

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

void init(const std::string &file_path);
void log(std::string_view logger_name, LogLevel level,
         std::string_view message);

} // namespace Lawnch::Logger
