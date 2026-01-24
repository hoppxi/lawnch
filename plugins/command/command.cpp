#include "PluginBase.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::string get_default_terminal() {
  const char *term = std::getenv("TERMINAL");
  if (term) {
    return term;
  }
  return "foot";
}

std::vector<lawnch::Result> do_cmd_query(const std::string &term) {
  std::string cmd = term;
  if (cmd.empty()) {
    lawnch::Result r;
    r.name = "Command Mode";
    r.comment = "Type a shell command to run";
    r.icon = "utilities-terminal";
    r.type = "cmd";
    return {r};
  }

  for (char &c : cmd) {
    if (std::iscntrl(static_cast<unsigned char>(c)))
      c = ' ';
  }

  static const std::string terminal = get_default_terminal();
  std::string full_command = terminal + " -e " + cmd;

  lawnch::Result r;
  r.name = cmd;
  r.comment = "Run in terminal";
  r.icon = "utilities-terminal";
  r.command = full_command;
  r.type = "cmd";
  return {r};
}

} // namespace

class CommandPlugin : public lawnch::Plugin {
public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);
    if (host && host->log_api) {
      host->log_api->log("CommandPlugin", LAWNCH_LOG_INFO, "Initialized");
    }
  }

  std::vector<std::string> get_triggers() override { return {":cmd", ">"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":cmd / >";
    r.comment = "Run a shell command";
    r.icon = "utilities-terminal";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    return do_cmd_query(term);
  }
};

LAWNCH_PLUGIN_DEFINE(CommandPlugin)
