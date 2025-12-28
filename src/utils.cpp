#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include "utils.hpp"
#include <array>
#include <memory>
#include <sstream>
#include <iostream>
#include <cstring>

namespace Utils {

    std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    void exec_detached(const std::string& cmd) {
        pid_t pid = fork();
        if (pid < 0) return;

        if (pid == 0) {
            setsid();

            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                if (devnull > STDERR_FILENO)
                    close(devnull);
            }

            execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
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


    std::string get_default_terminal() {
        const char* term_env = getenv("TERMINAL");
        if (term_env) return std::string(term_env);

        std::vector<std::string> common_terms = {
            "alacritty", "kitty", "gnome-terminal", "konsole", 
            "xfce4-terminal", "foot", "termit", "xterm"
        };

        for (const auto& t : common_terms) {
            if (system((std::string("command -v ") + t + " > /dev/null 2>&1").c_str()) == 0) {
                return t;
            }
        }
        return "xterm";
    }

    std::string get_default_editor() {
        const char* visual = getenv("VISUAL"); if (visual) return visual;
        const char* editor = getenv("EDITOR"); if (editor) return editor;
        for (auto e : {
            "nvim", "vim", "nano", "vi", "hx", "code"
        }) 
        if (system(("command -v " + std::string(e) + " >/dev/null").c_str()) == 0) return e;
        return "vi";
    }

    std::string resolve_path(std::string path) {
        if (path.empty()) return "";
        if (path[0] == '~') {
            path.replace(0, 1, get_home_dir());
        }
        return path;
    }

    std::string get_config_dir() {
        if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
            if (*xdg) return xdg;
        }

        if (const char* home = std::getenv("HOME")) {
            if (*home) return std::string(home) + "/.config";
        }

        if (const char* dirs = std::getenv("XDG_CONFIG_DIRS")) {
            std::stringstream ss(dirs);
            std::string dir;
            if (std::getline(ss, dir, ':') && !dir.empty()) return dir;
        }

        return "/etc/xdg";
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