#include "adapter.hpp"

namespace Lawnch::Core::Search::Plugins {

Adapter::Adapter(LawnchPluginVTable *vt) : vtable(vt) {
  if (vtable && vtable->plugin_api_version >= 5) {
    flags = vtable->flags;
  }
}

bool Adapter::allow_history() const {
  return !(flags & LAWNCH_PLUGIN_FLAG_DISABLE_HISTORY);
}

bool Adapter::is_custom_sorted() const {
  return (flags & LAWNCH_PLUGIN_FLAG_DISABLE_SORT);
}

Adapter::~Adapter() {
  if (vtable && vtable->destroy) {
    vtable->destroy();
  }
}

void Adapter::init_with_api(const LawnchHostApi *host_api) {
  if (vtable && vtable->init) {
    vtable->init(host_api);
  }
}

std::vector<std::string> Adapter::get_triggers() const {
  if (triggers_cached) {
    return cached_triggers;
  }
  cached_triggers.clear();
  if (vtable && vtable->get_triggers) {
    const char **triggers = vtable->get_triggers();
    while (triggers && *triggers) {
      cached_triggers.emplace_back(*triggers);
      triggers++;
    }
  }
  triggers_cached = true;
  return cached_triggers;
}

SearchResult Adapter::get_help() const {
  if (help_cached) {
    return cached_help;
  }

  if (vtable && vtable->get_help) {
    LawnchResult *r_ptr = vtable->get_help();
    if (!r_ptr) {
      cached_help = SearchMode::get_help();
      help_cached = true;
      return cached_help;
    }
    LawnchResult r = *r_ptr;
    cached_help = SearchResult{
        r.name ? r.name : "",
        r.comment ? r.comment : "",
        r.icon ? r.icon : "",
        r.command ? r.command : "",
        r.type ? r.type : "",
        r.preview_image_path ? r.preview_image_path : "",
        0,
    };
    if (vtable->free_results) {
      vtable->free_results(r_ptr, 1);
    }
    help_cached = true;
    return cached_help;
  }
  cached_help = SearchMode::get_help();
  help_cached = true;
  return cached_help;
}

std::vector<SearchResult> Adapter::query(const std::string &term) {
  std::vector<SearchResult> results;
  if (vtable && vtable->query) {
    int count = 0;
    LawnchResult *res = vtable->query(term.c_str(), &count);
    results.reserve(count);
    for (int i = 0; i < count; ++i) {
      results.push_back(SearchResult{
          res[i].name ? res[i].name : "", res[i].comment ? res[i].comment : "",
          res[i].icon ? res[i].icon : "", res[i].command ? res[i].command : "",
          res[i].type ? res[i].type : "",
          res[i].preview_image_path ? res[i].preview_image_path : "", 0,
          allow_history(), is_custom_sorted()});
    }
    if (vtable->free_results) {
      vtable->free_results(res, count);
    }
  }
  return results;
}

} // namespace Lawnch::Core::Search::Plugins
