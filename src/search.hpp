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

struct MapEntry {
    std::string name;
    std::string trigger;
    std::string trigger_short;
    std::string command_template; // contains {}
    std::string folder;           // for files mapping
    std::string exclude_type;
    int limit = 0;
};

struct Extension {
    std::string name;
    std::string trigger;
    std::string trigger_short;
    std::string path;           // script path
    std::string command;        // one-off command
    std::string on_select;      // command to run on selection
    std::string from_folder;    // folder to iterate
    std::string exclude_type;
    std::string help_text;
    std::string list_cmd;       // static list generation
    std::string icon_cmd;       // icon list generation
    bool json_output = false;
    int limit = 0;
};

using ResultsCallback = std::function<void(const std::vector<SearchResult>&)>;

class SearchEngine {
public:
    static SearchEngine& get();
    SearchEngine();

    // Main query functions
    void set_async_callback(ResultsCallback callback);
    std::vector<SearchResult> query(const std::string& term);
    void query_async(const std::string& term, ResultsCallback callback);

private:
    ResultsCallback async_callback = nullptr;

    // Core searchers
    std::vector<SearchResult> search_apps(const std::string& term, const MapEntry* map = nullptr);
    std::vector<SearchResult> search_bins(const std::string& term);
    std::vector<SearchResult> search_clipboard(const std::string& term);
    std::vector<SearchResult> search_calc(const std::string& term);
    std::vector<SearchResult> search_emoji(const std::string& term);
    std::vector<SearchResult> search_files(const std::string& term, const MapEntry* map = nullptr);
    std::vector<SearchResult> search_cmd(const std::string& term);
    std::vector<SearchResult> search_google_sync(const std::string& term);
    std::vector<SearchResult> search_youtube_sync(const std::string& term);
    std::vector<SearchResult> search_url(const std::string& term);
    std::vector<SearchResult> search_extensions(const std::string& term, const Extension& ext);
    std::vector<SearchResult> get_help(const std::string& term);

    // Helpers
    static std::string get_default_terminal();
    static std::string get_default_editor();
    void load_config();
    void load_emojis();
    std::string resolve_path(std::string path);
    bool is_trigger_match(const std::string& term, const std::string& trigger, const std::string& short_trigger, std::string& out_query);

    // Data
    std::vector<Extension> extensions;
    std::map<std::string, std::vector<MapEntry>> mapped_builtins;
    nlohmann::json emoji_cache;
    bool emoji_loaded = false;
};