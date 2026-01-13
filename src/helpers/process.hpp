#pragma once
#include <string>

namespace Lawnch::Proc {

bool is_executable(const std::string &path);
bool has_command(const std::string &cmd);

std::string get_default_terminal();
std::string get_default_editor();
std::string exec(const char *cmd);

void exec_detached(const std::string &cmd);

} // namespace Lawnch::Proc
