#include "lawnch_plugin_api.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

std::string get_default_terminal() {
  const char *term = std::getenv("TERMINAL");
  if (term) {
    return term;
  }
  return "foot"; // simple, could use the Utils::get_default_terminal function
}

std::vector<LawnchResult> do_cmd_query(const std::string &term) {
  std::string cmd = term;
  if (cmd.empty()) {
    return {{c_strdup("Command Mode"), c_strdup("Type a shell command to run"),
             c_strdup("utilities-terminal"), c_strdup(""), c_strdup("cmd"),
             c_strdup("")}};
  }

  for (char &c : cmd) {
    if (std::iscntrl(static_cast<unsigned char>(c)))
      c = ' ';
  }

  static const std::string terminal = get_default_terminal();
  std::string full_command = terminal + " -e " + cmd;

  return {{c_strdup(cmd.c_str()), c_strdup("Run in terminal"),
           c_strdup("utilities-terminal"), c_strdup(full_command.c_str()),
           c_strdup("cmd"), c_strdup("")}};
}

} // namespace

void plugin_init(const LawnchHostApi *host) {}
void plugin_destroy(void) {}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":cmd", ">", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *result = new LawnchResult;
  result->name = c_strdup(":cmd / >");
  result->comment = c_strdup("Run a shell command");
  result->icon = c_strdup("utilities-terminal");
  result->command = c_strdup("");
  result->type = c_strdup("help");
  result->preview_image_path = c_strdup("");
  return result;
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
    delete[] results[i].preview_image_path;
  }
  delete[] results;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  auto results_vec = do_cmd_query(term);
  *num_results = results_vec.size();
  if (*num_results == 0) {
    return nullptr;
  }
  LawnchResult *result_arr = new LawnchResult[*num_results];
  for (size_t i = 0; i < results_vec.size(); ++i) {
    result_arr[i] = results_vec[i];
  }
  return result_arr;
}

static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }
