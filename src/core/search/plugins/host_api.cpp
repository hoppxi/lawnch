#include "host_api.hpp"
#include "../../../helpers/fs.hpp"
#include "../../../helpers/logger.hpp"
#include "../../../helpers/search.hpp"
#include "../../../helpers/string.hpp"
#include "manager.hpp"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace Lawnch::Core::Search::Plugins {

// helpers
static char *copy_str(const std::string &s) {
  char *buf = (char *)malloc(s.size() + 1);
  if (buf)
    std::strcpy(buf, s.c_str());
  return buf;
}

static char **copy_str_list(const std::vector<std::string> &list, int *count) {
  if (count)
    *count = list.size();
  char **arr = (char **)malloc(sizeof(char *) * list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    arr[i] = copy_str(list[i]);
  }
  return arr;
}

static void s_free_path(char *p) { free(p); }
static void s_free_str_array(char **arr, int count) {
  if (!arr)
    return;
  for (int i = 0; i < count; ++i)
    free(arr[i]);
  free(arr);
}
static void s_free_str(char *s) { free(s); }

// host

const char *host_get_config_value(const LawnchHostApi *host, const char *key) {
  if (!host || !host->userdata || !key) {
    return nullptr;
  }
  auto *context = static_cast<PluginApiContext *>(host->userdata);

  std::string full_key = context->plugin_name + "." + key;

  auto &plugin_configs_map = context->config.plugin_configs;
  auto it = plugin_configs_map.find(full_key);
  if (it != plugin_configs_map.end()) {
    return it->second.c_str();
  }

  return nullptr;
}

const char *host_get_data_dir(const LawnchHostApi *host) {
  if (!host || !host->userdata) {
    return nullptr;
  }
  auto *context = static_cast<PluginApiContext *>(host->userdata);
  static thread_local std::string data_dir_cache;
  data_dir_cache = context->manager->get_plugin_data_dir(context->plugin_name);
  return data_dir_cache.c_str();
}

void host_request_results_update(const LawnchHostApi *host,
                                 const char *new_query) {
  if (!host || !host->userdata)
    return;
  auto *context = static_cast<PluginApiContext *>(host->userdata);
  if (context->manager) {
    if (host->log_api) {
      host->log_api->log("PluginManager", LAWNCH_LOG_DEBUG,
                         "s_request_results_update received call from plugin");
    }
    context->manager->fire_update_callback(context->plugin_name,
                                           new_query ? new_query : "");
  }
}

// logger
static void s_log(const char *name, LawnchLogLevel level, const char *msg) {
  Lawnch::Logger::LogLevel l = Lawnch::Logger::LogLevel::INFO;
  switch (level) {
  case LAWNCH_LOG_CRITICAL:
    l = Lawnch::Logger::LogLevel::CRITICAL;
    break;
  case LAWNCH_LOG_ERROR:
    l = Lawnch::Logger::LogLevel::ERROR;
    break;
  case LAWNCH_LOG_WARNING:
    l = Lawnch::Logger::LogLevel::WARNING;
    break;
  case LAWNCH_LOG_INFO:
    l = Lawnch::Logger::LogLevel::INFO;
    break;
  case LAWNCH_LOG_DEBUG:
    l = Lawnch::Logger::LogLevel::DEBUG;
    break;
  }
  Lawnch::Logger::log(name ? name : "Plugin", l, msg ? msg : "");
}

const LawnchLogApi g_log_api = {.log = s_log};

// fs
static char *s_fs_get_home() {
  return copy_str(Lawnch::Fs::get_home_path().string());
}
static char *s_fs_expand_tilde(const char *path) {
  return copy_str(Lawnch::Fs::expand_tilde(path ? path : "").string());
}
static char *s_fs_get_config_home() {
  return copy_str(Lawnch::Fs::get_config_home().string());
}
static char *s_fs_get_data_home() {
  return copy_str(Lawnch::Fs::get_data_home().string());
}
static char *s_fs_get_cache_home() {
  return copy_str(Lawnch::Fs::get_cache_home().string());
}
static char *s_fs_get_log_path(const char *app) {
  return copy_str(Lawnch::Fs::get_log_path(app ? app : "").string());
}
static char *s_fs_get_socket_path(const char *fname) {
  return copy_str(Lawnch::Fs::get_socket_path(fname ? fname : "").string());
}
static char **s_fs_get_data_dirs(int *cnt) {
  return copy_str_list(Lawnch::Fs::get_data_dirs(), cnt);
}
static char **s_fs_get_icon_dirs(int *cnt) {
  return copy_str_list(Lawnch::Fs::get_icon_dirs(), cnt);
}

const LawnchFsApi g_fs_api = {.get_home_path = s_fs_get_home,
                              .expand_tilde = s_fs_expand_tilde,
                              .get_config_home = s_fs_get_config_home,
                              .get_data_home = s_fs_get_data_home,
                              .get_cache_home = s_fs_get_cache_home,
                              .get_log_path = s_fs_get_log_path,
                              .get_socket_path = s_fs_get_socket_path,
                              .get_data_dirs = s_fs_get_data_dirs,
                              .get_icon_dirs = s_fs_get_icon_dirs,
                              .free_path = s_free_path,
                              .free_str_array = s_free_str_array};

// string
static char *s_str_trim(const char *s) {
  return copy_str(Lawnch::Str::trim(s ? s : ""));
}
static char *s_str_to_lower(const char *s) {
  return copy_str(Lawnch::Str::to_lower_copy(s ? s : ""));
}
static char *s_str_unescape(const char *s) {
  return copy_str(Lawnch::Str::unescape(s ? s : ""));
}
static char *s_str_escape(const char *s) {
  return copy_str(Lawnch::Str::escape(s ? s : ""));
}
static char *s_str_replace_all(const char *s, const char *from,
                               const char *to) {
  return copy_str(
      Lawnch::Str::replace_all(s ? s : "", from ? from : "", to ? to : ""));
}
static char **s_str_tokenize(const char *s, char delim, int *cnt) {
  return copy_str_list(Lawnch::Str::tokenize(s ? s : "", delim), cnt);
}
static int s_str_iequals(const char *a, const char *b) {
  return Lawnch::Str::iequals(a ? a : "", b ? b : "");
}
static int s_str_contains_ic(const char *h, const char *n) {
  return Lawnch::Str::contains_ic(h ? h : "", n ? n : "");
}
static int s_str_match_score(const char *i, const char *t) {
  return Lawnch::Str::match_score(i ? i : "", t ? t : "");
}
static size_t s_str_hash(const char *s) {
  return Lawnch::Str::hash(s ? s : "");
}

const LawnchStrApi g_str_api = {.trim = s_str_trim,
                                .to_lower_copy = s_str_to_lower,
                                .unescape = s_str_unescape,
                                .escape = s_str_escape,
                                .replace_all = s_str_replace_all,
                                .tokenize = s_str_tokenize,
                                .iequals = s_str_iequals,
                                .contains_ic = s_str_contains_ic,
                                .match_score = s_str_match_score,
                                .hash = s_str_hash,
                                .free_str = s_free_str,
                                .free_str_array = s_free_str_array};

// search
static int s_search_fuzzy_match(const char *pattern, const char *str) {
  return Lawnch::Helpers::Search::fuzzy_match(pattern ? pattern : "",
                                              str ? str : "");
}
static int s_search_multi_word_match(const char *pattern, const char *str) {
  return Lawnch::Helpers::Search::multi_word_match(pattern ? pattern : "",
                                                   str ? str : "");
}
static int s_search_levenshtein_distance(const char *s1, const char *s2) {
  return Lawnch::Helpers::Search::levenshtein_distance(s1 ? s1 : "",
                                                       s2 ? s2 : "");
}
static double s_search_jaro_winkler_similarity(const char *s1, const char *s2) {
  return Lawnch::Helpers::Search::jaro_winkler_similarity(s1 ? s1 : "",
                                                          s2 ? s2 : "");
}
static int s_search_calculate_advanced_score(const char *query,
                                             const char *target) {
  return Lawnch::Helpers::Search::calculate_advanced_score(
      query ? query : "", target ? target : "");
}

const LawnchSearchApi g_search_api = {
    .fuzzy_match = s_search_fuzzy_match,
    .multi_word_match = s_search_multi_word_match,
    .levenshtein_distance = s_search_levenshtein_distance,
    .jaro_winkler_similarity = s_search_jaro_winkler_similarity,
    .calculate_advanced_score = s_search_calculate_advanced_score};

} // namespace Lawnch::Core::Search::Plugins
