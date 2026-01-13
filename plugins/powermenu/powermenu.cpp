#include "../lawnch_plugin_api.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
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

// example plugin with config
// Global state for the plugin
struct PluginState {
  std::map<std::string, PowerOption> options;
  std::vector<std::string> option_order;
};

static PluginState g_state;

// Helper to duplicate a C++ string to a C string
static char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

// Helper to get a config value with a fallback
const char *get_config_or_null(const LawnchHostApi *host, const char *key) {
  if (!host || !host->get_config_value)
    return nullptr;
  return host->get_config_value(host, key);
}

void load_defaults() {
  g_state.options.clear();
  g_state.option_order.clear();

  // Default options and order
  g_state.options["shutdown"] = {"Shutdown", "Power off the computer",
                                 "system-shutdown-symbolic", "shutdown -h now"};
  g_state.options["reboot"] = {"Reboot", "Restart the computer",
                               "system-reboot-symbolic", "reboot"};
  g_state.options["suspend"] = {"Suspend", "Suspend the computer",
                                "system-suspend-symbolic", "systemctl suspend"};
  g_state.options["hibernate"] = {"Hibernate", "Save session to disk",
                                  "system-hibernate-symbolic",
                                  "systemctl hibernate"};
  g_state.options["log-out"] = {
      "Log Out", "Log out of your session", "system-log-out-symbolic",
      "loginctl terminate-session ${XDG_SESSION_ID-}"};
  g_state.options["lockscreen"] = {"Lockscreen", "Lock the screen",
                                   "lock-symbolic",
                                   "pidof hyprlock || hyprlock"};

  g_state.option_order = {"shutdown",  "reboot",  "suspend",
                          "hibernate", "log-out", "lockscreen"};
}

} // namespace

// C-API Implementation
void plugin_init(const LawnchHostApi *host) {
  load_defaults();

  // Override order if specified in config
  const char *order_str = get_config_or_null(host, "order");
  if (order_str) {
    g_state.option_order.clear();
    std::stringstream ss(order_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
      // trim whitespace
      item.erase(0, item.find_first_not_of(" \t\n\r"));
      item.erase(item.find_last_not_of(" \t\n\r") + 1);
      if (!item.empty()) {
        g_state.option_order.push_back(item);
      }
    }
  }

  // For each option (either default or from config order), override properties
  // from config
  for (const auto &option_key : g_state.option_order) {
    // ensure a default entry exists to be overwritten
    if (g_state.options.find(option_key) == g_state.options.end()) {
      g_state.options[option_key] = {option_key, "", "", ""};
    }

    PowerOption &option = g_state.options[option_key];

    const char *name_val =
        get_config_or_null(host, (option_key + "_name").c_str());
    if (name_val)
      option.name = name_val;

    const char *comment_val =
        get_config_or_null(host, (option_key + "_comment").c_str());
    if (comment_val)
      option.comment = comment_val;

    const char *icon_val =
        get_config_or_null(host, (option_key + "_icon").c_str());
    if (icon_val)
      option.icon = icon_val;

    const char *command_val =
        get_config_or_null(host, (option_key + "_command").c_str());
    if (command_val)
      option.command = command_val;
  }
}

void plugin_destroy(void) {
  g_state.options.clear();
  g_state.option_order.clear();
}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":power", ":p", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *result = new LawnchResult;
  result->name = c_strdup(":power / :p");
  result->comment = c_strdup("Power options (shutdown, reboot, etc.)");
  result->icon = c_strdup("system-shutdown-symbolic");
  result->command = c_strdup("");
  result->type = c_strdup("help");
  return result;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  std::string lower_term = term;
  std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::vector<LawnchResult> results;

  for (const auto &option_name : g_state.option_order) {
    const auto &option = g_state.options.at(option_name);

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
      results.push_back({
          c_strdup(option.name),
          c_strdup(option.comment),
          c_strdup(option.icon),
          c_strdup(option.command),
          c_strdup("plugin"),
      });
    }
  }

  *num_results = results.size();
  if (*num_results == 0) {
    return nullptr;
  }

  LawnchResult *result_arr = new LawnchResult[*num_results];
  for (size_t i = 0; i < results.size(); ++i) {
    result_arr[i] = results[i];
  }
  return result_arr;
}

void plugin_free_results(LawnchResult *results, int num_results) {
  if (!results)
    return;
  for (int i = 0; i < num_results; ++i) {
    delete[] results[i].name;
    delete[] results[i].comment;
    delete[] results[i].icon;
    delete[] results[i].command;
    delete[] results[i].type;
  }
  delete[] results;
}

// VTable and entry point
static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }
