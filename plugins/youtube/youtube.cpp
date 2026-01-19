#include "lawnch_plugin_api.h"
#include <cstring>
#include <curl/curl.h>
#include <string>
#include <vector>

namespace {
char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

std::string url_encode(const std::string &value) {
  CURL *curl = curl_easy_init();
  if (curl) {
    char *output = curl_easy_escape(curl, value.c_str(), value.length());
    if (output) {
      std::string result(output);
      curl_free(output);
      curl_easy_cleanup(curl);
      return result;
    }
    curl_easy_cleanup(curl);
  }
  return "";
}

std::vector<LawnchResult> do_youtube_query(const std::string &term) {
  std::string display = term.empty() ? "Type to search YouTube..." : term;
  std::string encoded_term = url_encode(term);
  std::string command =
      "xdg-open \"https://www.youtube.com/results?search_query=" +
      encoded_term + "\"";

  return {{
      c_strdup(display.c_str()),
      c_strdup("Search YouTube (Enter to open)"),
      c_strdup("multimedia-video-player"),
      c_strdup(command.c_str()),
      c_strdup("youtube"),
      c_strdup(""),
  }};
}

} // namespace

void plugin_init(const LawnchHostApi *host) {}
void plugin_destroy(void) {}

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":youtube", ":yt", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  auto result = new LawnchResult;
  result->name = c_strdup(":youtube / :yt");
  result->comment = c_strdup("Search YouTube");
  result->icon = c_strdup("multimedia-video-player");
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
  auto results_vec = do_youtube_query(term);
  *num_results = results_vec.size();
  if (*num_results == 0) {
    return nullptr;
  }
  auto result_arr = new LawnchResult[*num_results];
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
