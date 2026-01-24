#pragma once

#include "lawnch_plugin_api.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace Lawnch {

inline char *strdup(const std::string &s) {
  char *c = new char[s.length() + 1];
  std::memcpy(c, s.c_str(), s.length() + 1);
  return c;
}

struct Result {
  std::string name;
  std::string comment;
  std::string icon;
  std::string command;
  std::string type;
  std::string preview_image_path;

  LawnchResult to_c() const {
    LawnchResult r;
    r.name = strdup(name);
    r.comment = strdup(comment);
    r.icon = strdup(icon);
    r.command = strdup(command);
    r.type = strdup(type);
    r.preview_image_path = strdup(preview_image_path);
    return r;
  }
};

class Plugin {
protected:
  const LawnchHostApi *host_ = nullptr;

public:
  virtual ~Plugin() = default;

  virtual void init(const LawnchHostApi *host) { host_ = host; }

  virtual void destroy() {}

  virtual std::vector<std::string> get_triggers() = 0;

  virtual Result get_help() = 0;

  virtual std::vector<Result> query(const std::string &term) = 0;

  virtual uint32_t get_flags() const { return 0; }
};

inline Plugin *g_plugin_instance = nullptr;

} // namespace Lawnch

extern "C" {

static void plugin_init(const LawnchHostApi *host) {
  if (Lawnch::g_plugin_instance) {
    Lawnch::g_plugin_instance->init(host);
  }
}

static void plugin_destroy() {
  if (Lawnch::g_plugin_instance) {
    Lawnch::g_plugin_instance->destroy();
  }
}

static const char **plugin_get_triggers() {
  static std::vector<const char *> c_triggers;
  static std::vector<std::string> s_triggers;

  if (Lawnch::g_plugin_instance) {
    s_triggers = Lawnch::g_plugin_instance->get_triggers();
    c_triggers.clear();
    for (const auto &s : s_triggers) {
      c_triggers.push_back(s.c_str());
    }
    c_triggers.push_back(nullptr);
    return c_triggers.data();
  }
  static const char *empty[] = {nullptr};
  return empty;
}

static LawnchResult *plugin_get_help() {
  if (Lawnch::g_plugin_instance) {
    Lawnch::Result r = Lawnch::g_plugin_instance->get_help();
    LawnchResult *cr = new LawnchResult(r.to_c());
    return cr;
  }
  return nullptr;
}

static LawnchResult *plugin_query(const char *term, int *num_results) {
  if (Lawnch::g_plugin_instance && term) {
    // Catch exceptions to prevent crashing the host
    try {
      std::vector<Lawnch::Result> results =
          Lawnch::g_plugin_instance->query(term);
      *num_results = results.size();
      if (results.empty())
        return nullptr;

      LawnchResult *arr = new LawnchResult[results.size()];
      for (size_t i = 0; i < results.size(); ++i) {
        arr[i] = results[i].to_c();
      }
      return arr;
    } catch (...) {
      *num_results = 0;
      return nullptr;
    }
  }
  *num_results = 0;
  return nullptr;
}

static void plugin_free_results(LawnchResult *results, int num_results) {
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

static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results,
                                      .flags = 0};

} // extern "C"

#define LAWNCH_PLUGIN_DEFINE(PluginClass)                                      \
  extern "C" PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) {        \
    static PluginClass instance;                                               \
    Lawnch::g_plugin_instance = &instance;                                     \
    g_vtable.flags = instance.get_flags();                                     \
    return &g_vtable;                                                          \
  }
