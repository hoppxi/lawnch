#include "process.hpp"
#include <array>
#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Lawnch::Proc {

bool is_executable(const std::string &path) {
  struct stat sb;
  return (stat(path.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR));
}

bool has_command(const std::string &cmd) {
  std::string check = "command -v " + cmd + " > /dev/null 2>&1";
  return std::system(check.c_str()) == 0;
}

std::string get_default_terminal() {
  // only computed once per program run
  static std::string cached_term = []() -> std::string {
    if (const char *term = std::getenv("TERMINAL"))
      return term;

    const std::vector<std::string> terms = {
        "alacritty",      "kitty", "gnome-terminal", "konsole",
        "xfce4-terminal", "foot",  "termit",         "xterm"};

    for (const auto &t : terms) {
      if (has_command(t))
        return t;
    }
    return "xterm"; // fallback
  }();

  return cached_term;
}

std::string get_default_editor() {
  static std::string cached_editor = []() -> std::string {
    if (const char *vis = std::getenv("VISUAL"))
      return vis;
    if (const char *ed = std::getenv("EDITOR"))
      return ed;

    const std::vector<std::string> editors = {"nvim", "vim", "nano", "vi",
                                              "code"};
    for (const auto &e : editors) {
      if (has_command(e))
        return e;
    }
    return "vi";
  }();

  return cached_editor;
}

std::string exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    return "";
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

void exec_detached(const std::string &cmd) {
  pid_t pid = fork();
  if (pid < 0)
    return;

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

    execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
    _exit(127);
  }
}

} // namespace Lawnch::Proc
