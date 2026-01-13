#pragma once
#include "../interface.hpp"

namespace Lawnch::Core::Search::Providers {

class AppMode : public SearchMode {
public:
  std::vector<std::string> get_triggers() const override {
    return {":apps", ":a"};
  }
  std::vector<SearchResult> query(const std::string &term) override;
  SearchResult get_help() const override {
    return {":apps / :a", "Search installed applications", "system-search", "",
            "help"};
  }
};

class BinMode : public SearchMode {
public:
  std::vector<std::string> get_triggers() const override {
    return {":bin", ":b"};
  }
  std::vector<SearchResult> query(const std::string &term) override;
  SearchResult get_help() const override {
    return {":bin / :b", "Search executables in PATH", "utilities-terminal", "",
            "help"};
  }
};

} // namespace Lawnch::Core::Search::Providers
