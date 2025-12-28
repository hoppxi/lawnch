#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace Utils {
    std::string exec(const char* cmd);
    void exec_detached(const std::string& cmd);
    std::string get_home_dir();
    std::string get_default_terminal();
    std::string get_default_editor();
    std::string resolve_path(std::string path);
    bool is_executable(const std::string& path);
    int match_score(const std::string& input, const std::string& target);
    std::vector<std::string> tokenize(const std::string& str);    
    bool contains_ignore_case(const std::string& haystack, const std::string& needle);
}

