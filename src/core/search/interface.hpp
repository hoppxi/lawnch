#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Lawnch::Core::Search {

struct SearchResult {
  std::string name;
  std::string comment;
  std::string icon;
  std::string command;
  std::string type;
  std::string preview_image_path;
  int score = 0;
};

using ResultsCallback = std::function<void(const std::vector<SearchResult> &)>;

class SearchMode {
public:
  virtual ~SearchMode() = default;
  virtual std::vector<std::string> get_triggers() const = 0;
  virtual std::vector<SearchResult> query(const std::string &term) = 0;
  virtual SearchResult get_help() const {
    auto t = get_triggers();
    std::string primary = t.empty() ? "" : t[0];
    return {primary, "Search mode", "help-about", "", "help", "", 0};
  }
  virtual void init() {}
  virtual void set_async_callback(ResultsCallback callback) {
    async_callback = callback;
  }

protected:
  ResultsCallback async_callback = nullptr;
};

} // namespace Lawnch::Core::Search
