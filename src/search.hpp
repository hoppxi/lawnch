#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <glib.h>

struct SearchResult {
    std::string name;
    std::string comment;
    std::string icon;
    std::string command;
    std::string type;
    int score = 0;
};

using ResultsCallback = std::function<void(const std::vector<SearchResult>&)>;

class SearchEngine {
public:
    static SearchEngine& get();
    SearchEngine();

    void set_async_callback(ResultsCallback callback);
    std::vector<SearchResult> query(const std::string& term);
    void query_async(const std::string& term, ResultsCallback callback);

private:
    ResultsCallback async_callback = nullptr;

    std::vector<SearchResult> search_apps(const std::string& term);
    std::vector<SearchResult> search_bins(const std::string& term);
    std::vector<SearchResult> search_clipboard(const std::string& term);
    std::vector<SearchResult> search_calc(const std::string& term);
    std::vector<SearchResult> search_emoji(const std::string& term);
    std::vector<SearchResult> search_files(const std::string& term);
    std::vector<SearchResult> search_cmd(const std::string& term);
    std::vector<SearchResult> search_google_sync(const std::string& term);
    std::vector<SearchResult> search_youtube_sync(const std::string& term);
    std::vector<SearchResult> search_url(const std::string& term);
    std::vector<SearchResult> get_help(const std::string& term);

    void load_emojis();
    bool is_trigger_match(const std::string& term, const std::string& trigger, const std::string& short_trigger, std::string& out_query);

    nlohmann::json emoji_cache;
    bool emoji_loaded = false;
};