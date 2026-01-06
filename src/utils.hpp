#pragma once
#include <algorithm>
#include <cairo.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Utils {
enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };
void log(const std::string &logger_name, LogLevel level,
         const std::string &message);
void draw_rounded_rect(cairo_t *cr, double x, double y, double width,
                       double height, double radius);
void exec_detached(const std::string &cmd);

std::string get_home_dir();
std::string get_default_terminal();
std::string get_default_editor();
std::string resolve_path(std::string path);
std::string get_socket_path();
std::string get_log_path();
std::string get_xdg_config_home();
std::string get_xdg_data_home();
std::string get_cache_dir();

bool is_executable(const std::string &path);
bool contains_ignore_case(const std::string &haystack,
                          const std::string &needle);

int match_score(const std::string &input, const std::string &target);

std::string exec(const char *cmd);
std::vector<std::string> tokenize(const std::string &str);

} // namespace Utils
