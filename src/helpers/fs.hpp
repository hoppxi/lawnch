#pragma once
#include <filesystem>
#include <string>

namespace Lawnch::Fs {

std::filesystem::path get_home_path();
std::filesystem::path expand_tilde(const std::string &path_str);
std::filesystem::path get_config_home();
std::filesystem::path get_data_home();
std::filesystem::path get_cache_dir();
std::filesystem::path get_log_path(const std::string &app_name);
std::filesystem::path get_socket_path(const std::string &filename);

} // namespace Lawnch::Fs
