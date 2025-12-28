#include "utils.hpp"
#include <array>
#include <memory>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sstream>
#include <iostream>
#include <cstring>

namespace Utils {

    std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    void exec_detached(const std::string& cmd) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setsid(); // Create new session
            // Redirect stdin/out/err to /dev/null
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
            
            execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)nullptr);
            _exit(127);
        }
    }

    std::string get_home_dir() {
        const char* home = getenv("HOME");
        if (home == nullptr) {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd) return pwd->pw_dir;
            return "";
        }
        return home;
    }

    bool is_executable(const std::string& path) {
        struct stat sb;
        return (stat(path.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR));
    }

    std::vector<std::string> tokenize(const std::string& str) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (ss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            tokens.push_back(token);
        }
        return tokens;
    }

    bool contains_ignore_case(const std::string& haystack, const std::string& needle) {
        auto it = std::search(
            haystack.begin(), haystack.end(),
            needle.begin(), needle.end(),
            [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
        );
        return (it != haystack.end());
    }

    int match_score(const std::string& input, const std::string& target) {
        if (input.empty()) return 1;
        std::string input_lower = input; 
        std::string target_lower = target;
        std::transform(input_lower.begin(), input_lower.end(), input_lower.begin(), ::tolower);
        std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);

        if (target_lower == input_lower) return 100;
        if (target_lower.find(input_lower) == 0) return 80; // Prefix match
        if (target_lower.find(input_lower) != std::string::npos) return 50; // Substring
        return 0;
    }
}