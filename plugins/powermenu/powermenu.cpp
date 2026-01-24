#include "PluginBase.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct PowerOption {
  std::string name;
  std::string comment;
  std::string icon;
  std::string command;
};

} // namespace

class PowermenuPlugin : public lawnch::Plugin {
private:
  std::map<std::string, PowerOption> options_;
  std::vector<std::string> option_order_;

  const char *get_config_or_null(const char *key) {
    if (!host_ || !host_->get_config_value)
      return nullptr;
    return host_->get_config_value(host_, key);
  }

  void load_defaults() {
    options_.clear();
    option_order_.clear();

    // Default options and order
    options_["shutdown"] = {"Shutdown", "Power off the computer",
                            "system-shutdown-symbolic", "shutdown -h now"};
    options_["reboot"] = {"Reboot", "Restart the computer",
                          "system-reboot-symbolic", "reboot"};
    options_["suspend"] = {"Suspend", "Suspend the computer",
                           "system-suspend-symbolic", "systemctl suspend"};
    options_["hibernate"] = {"Hibernate", "Save session to disk",
                             "system-hibernate-symbolic",
                             "systemctl hibernate"};
    options_["log-out"] = {"Log Out", "Log out of your session",
                           "system-log-out-symbolic",
                           "loginctl terminate-session ${XDG_SESSION_ID-}"};
    options_["lockscreen"] = {"Lockscreen", "Lock the screen", "lock-symbolic",
                              "pidof hyprlock || hyprlock"};

    option_order_ = {"shutdown",  "reboot",  "suspend",
                     "hibernate", "log-out", "lockscreen"};
  }

public:
  void init(const LawnchHostApi *host) override {
    Plugin::init(host);

    load_defaults();

    // Override order if specified in config
    const char *order_str = get_config_or_null("order");
    if (order_str) {
      option_order_.clear();
      std::stringstream ss(order_str);
      std::string item;
      while (std::getline(ss, item, ',')) {
        // trim whitespace
        item.erase(0, item.find_first_not_of(" \t\n\r"));
        item.erase(item.find_last_not_of(" \t\n\r") + 1);
        if (!item.empty()) {
          option_order_.push_back(item);
        }
      }
    }

    // For each option (either default or from config order), override
    // properties from config
    for (const auto &option_key : option_order_) {
      // ensure a default entry exists to be overwritten
      if (options_.find(option_key) == options_.end()) {
        options_[option_key] = {option_key, "", "", ""};
      }

      PowerOption &option = options_[option_key];

      const char *name_val = get_config_or_null((option_key + "_name").c_str());
      if (name_val)
        option.name = name_val;

      const char *comment_val =
          get_config_or_null((option_key + "_comment").c_str());
      if (comment_val)
        option.comment = comment_val;

      const char *icon_val = get_config_or_null((option_key + "_icon").c_str());
      if (icon_val)
        option.icon = icon_val;

      const char *command_val =
          get_config_or_null((option_key + "_command").c_str());
      if (command_val)
        option.command = command_val;
    }
  }

  void destroy() override {
    options_.clear();
    option_order_.clear();
  }

  std::vector<std::string> get_triggers() override { return {":power", ":p"}; }

  lawnch::Result get_help() override {
    lawnch::Result r;
    r.name = ":power / :p";
    r.comment = "Power options (shutdown, reboot, etc.)";
    r.icon = "system-shutdown-symbolic";
    r.type = "help";
    return r;
  }

  std::vector<lawnch::Result> query(const std::string &term) override {
    std::string lower_term = term;
    std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::vector<lawnch::Result> results;

    for (const auto &option_name : option_order_) {
      const auto &option = options_.at(option_name);

      bool match = false;
      if (lower_term.empty()) {
        match = true;
      } else {
        std::string name = option.name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (name.find(lower_term) != std::string::npos) {
          match = true;
        }
      }

      if (match) {
        lawnch::Result r;
        r.name = option.name;
        r.comment = option.comment;
        r.icon = option.icon;
        r.command = option.command;
        r.type = "plugin";
        results.push_back(r);
      }
    }
    return results;
  }

  uint32_t get_flags() const override {
    return LAWNCH_PLUGIN_FLAG_DISABLE_SORT;
  }
};

LAWNCH_PLUGIN_DEFINE(PowermenuPlugin)
