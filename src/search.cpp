#include "search.hpp"
#include "utils.hpp"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <sstream>
#include <algorithm>
#include <glib.h>
#include <curl/curl.h>

namespace fs = std::filesystem;

SearchEngine::SearchEngine() {}

SearchEngine& SearchEngine::get() {
    static SearchEngine instance;
    return instance;
}

void SearchEngine::set_async_callback(ResultsCallback callback) {
    async_callback = callback;
}

bool SearchEngine::is_trigger_match(const std::string& term, const std::string& trigger, const std::string& short_trigger, std::string& out_query) {
    auto check = [&](const std::string& t) {
        if (t.empty()) return false;
        if (term == t) { out_query = ""; return true; }
        if (term.rfind(t + " ", 0) == 0) {
            out_query = term.substr(t.length() + 1);
            return true;
        }
        return false;
    };
    return check(trigger) || check(short_trigger);
}

std::vector<SearchResult> SearchEngine::query(const std::string& term) {
    if (term.empty()) return {};

    std::string sub_q;
    if (is_trigger_match(term, ":help", ":h", sub_q)) return get_help(sub_q);
    if (is_trigger_match(term, ":bin", ":b", sub_q)) return search_bins(sub_q);
    if (is_trigger_match(term, ":clip", ":c", sub_q)) return search_clipboard(sub_q);
    if (is_trigger_match(term, ":calc", "", sub_q)) return search_calc(sub_q);
    if (is_trigger_match(term, ":emoji", ":e", sub_q)) return search_emoji(sub_q);
    if (is_trigger_match(term, ":files", ":f", sub_q)) return search_files(sub_q);
    if (is_trigger_match(term, ":cmd", "", sub_q)) return search_cmd(sub_q);
    if (is_trigger_match(term, ":google", ":g", sub_q)) {
        if (async_callback) query_async(term, async_callback);
        return search_google_sync(sub_q);
    }
    if (is_trigger_match(term, ":youtube", ":yt", sub_q)) {
        if (async_callback) query_async(term, async_callback);
        return search_youtube_sync(sub_q);
    }
    if (is_trigger_match(term, ":url", ":u", sub_q)) return search_url(sub_q);
    if (is_trigger_match(term, ":apps", ":a", sub_q)) return search_apps(sub_q);

    auto results = search_apps(term);
    if (results.empty()) {
        auto bins = search_bins(term);
        results.insert(results.end(), bins.begin(), bins.end());
    }
    return results;
}

std::vector<SearchResult> SearchEngine::get_help(const std::string& term) {
    std::vector<SearchResult> help;
    auto add_h = [&](std::string t, std::string s, std::string desc) {
        if (!term.empty() && !Utils::contains_ignore_case(t, term) && !Utils::contains_ignore_case(desc, term)) return;
        help.push_back({ t + (s.empty() ? "" : " / " + s), desc, "help-about", "", "help" });
    };

    add_h(":apps", ":a", "Search installed applications");
    add_h(":files", ":f", "Search files (use :dir 'path' to filter)");
    add_h(":bin", ":b", "Search executables in PATH");
    add_h(":clip", ":c", "Clipboard history");
    add_h(":calc", "", "Calculator (bc)");
    add_h(":emoji", ":e", "Emoji picker");
    add_h(":google", ":g", "Search the web");
    add_h(":youtube", ":yt", "Search YouTube");
    add_h(":url", ":u", "Open a URL directly in browser");
    add_h(":cmd", "", "Run a raw shell command");
    
    return help;
}

std::vector<SearchResult> SearchEngine::search_apps(const std::string& term) {
    std::vector<SearchResult> results;
    std::vector<std::string> data_dirs;
    
    const char* xdg_data = getenv("XDG_DATA_DIRS");
    std::string xdg = xdg_data ? xdg_data : "/usr/share:/usr/local/share";
    std::stringstream ss(xdg);
    std::string segment;
    while(std::getline(ss, segment, ':')) {
        data_dirs.push_back(segment + "/applications");
    }
    data_dirs.push_back(Utils::get_home_dir() + "/.local/share/applications");

    static std::string terminal_cmd = Utils::get_default_terminal();

    for (const auto& dir : data_dirs) {
        if (!fs::exists(dir)) continue;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() != ".desktop") continue;
            GKeyFile* key_file = g_key_file_new();
            if (g_key_file_load_from_file(key_file, entry.path().c_str(), G_KEY_FILE_NONE, NULL)) {
                if (!g_key_file_get_boolean(key_file, "Desktop Entry", "NoDisplay", NULL)) {
                    char* name = g_key_file_get_string(key_file, "Desktop Entry", "Name", NULL);
                    char* exec = g_key_file_get_string(key_file, "Desktop Entry", "Exec", NULL);
                    char* icon = g_key_file_get_string(key_file, "Desktop Entry", "Icon", NULL);
                    char* comment = g_key_file_get_string(key_file, "Desktop Entry", "Comment", NULL);

                    if (name && exec) {
                        std::string s_name(name);
                        int score = term.empty() ? 1 : Utils::match_score(term, s_name);

                        if (score > 0) {
                            std::string cmd(exec);
                            size_t pct = cmd.find('%');
                            if (pct != std::string::npos) cmd = cmd.substr(0, pct);

                            if (g_key_file_get_boolean(key_file, "Desktop Entry", "Terminal", NULL)) {
                                cmd = terminal_cmd + " -e " + cmd;
                            }

                            results.push_back({
                                s_name,
                                comment ? std::string(comment) : (""),
                                icon ? std::string(icon) : "application-x-executable",
                                cmd, "app", score
                            });
                        }
                    }
                    g_free(name); g_free(exec); g_free(icon); g_free(comment);
                }
            }
            g_key_file_free(key_file);
        }
    }
    
    std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b){
        if (a.score != b.score) return a.score > b.score;
        return a.name < b.name;
    });

    int limit = 50;
    if (results.size() > (size_t)limit) results.resize(limit);
    return results;
}

std::vector<SearchResult> SearchEngine::search_files(const std::string& term) {
    std::string search_dir = "~";
    std::string query = term;

    size_t dir_pos = term.find(":dir ");
    if (dir_pos != std::string::npos) {
        size_t start = term.find("'", dir_pos);
        size_t end = (start != std::string::npos) ? term.find("'", start + 1) : std::string::npos;
        
        if (start != std::string::npos && end != std::string::npos) {
            search_dir = term.substr(start + 1, end - start - 1);
            query = term.substr(end + 1);
            size_t first_char = query.find_first_not_of(" ");
            query = (first_char == std::string::npos) ? "" : query.substr(first_char);
        }
    }

    search_dir = Utils::resolve_path(search_dir);

    if (!fs::exists(search_dir)) search_dir = Utils::resolve_path("~");

    std::string cmd = "find \"" + search_dir + "\" -maxdepth 4 -not -path '*/.*' ";
    if (!query.empty()) cmd += "-iname '*" + query + "*' ";
    
    int limit = 50;
    cmd += "2>/dev/null | head -n " + std::to_string(limit);

    std::string out = Utils::exec(cmd.c_str());
    std::stringstream ss(out);
    std::string line;
    std::vector<SearchResult> results;

    static std::string editor_cmd = Utils::get_default_editor();
    static std::string terminal_cmd = Utils::get_default_terminal();

    while(std::getline(ss, line)) {
        if(line.empty()) continue;
        fs::path p(line);
        bool is_dir = fs::is_directory(p);
        
        std::string open_cmd;
        open_cmd = is_dir ? terminal_cmd + " -e " + editor_cmd + " \"" + line + "\"" : "xdg-open \"" + line + "\"";

        results.push_back({ 
            p.filename().string(), 
            line, 
            is_dir ? "folder" : "text-x-generic", 
            open_cmd, "file" 
        });
    }
    return results;
}

void SearchEngine::query_async(const std::string& term, ResultsCallback callback) {
    if (term.empty()) return;
    bool is_google = (term.rfind(":google ", 0) == 0 || term.rfind(":g ", 0) == 0);
    bool is_yt = (term.rfind(":youtube ", 0) == 0 || term.rfind(":yt ", 0) == 0);
    if (!is_google && !is_yt) return;

    std::thread([term, callback, is_google, is_yt]() {
        std::vector<SearchResult> results;
        size_t off = term.find(' ') + 1;
        std::string query = term.substr(off);
        if (query.empty()) return;

        CURL* curl = curl_easy_init();
        if (!curl) return;
        std::string readBuffer;
        std::string baseUrl = is_google ? "https://google.com/complete/search?client=firefox" : "https://google.com/complete/search?client=firefox&ds=yt";
        char* enc = curl_easy_escape(curl, query.c_str(), query.length());
        std::string fullUrl = baseUrl + "&q=" + enc;
        curl_free(enc);

        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* c, size_t s, size_t n, void* u){ ((std::string*)u)->append((char*)c, s*n); return s*n; });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "lawnch/1.0");

        if (curl_easy_perform(curl) == CURLE_OK && !readBuffer.empty()) {
            try {
                auto j = nlohmann::json::parse(readBuffer);
                if (j.is_array() && j.size() >= 2 && j[1].is_array()) {
                    for (const auto& sug : j[1]) {
                        std::string s = sug.get<std::string>();
                        std::string cmd = is_google ? "xdg-open \"https://google.com/search?q=" + s + "\"" : "xdg-open \"https://youtube.com/results?search_query=" + s + "\"";
                        results.push_back({ s, "Suggestion", is_google ? "system-search" : "video-x-generic", cmd, "suggestion", 50 });
                    }
                }
            } catch (...) {}
        }
        curl_easy_cleanup(curl);
        if (!results.empty()) {
            struct D { ResultsCallback c; std::vector<SearchResult> r; };
            g_idle_add([](gpointer p){ auto* d=(D*)p; d->c(d->r); delete d; return (gboolean)FALSE; }, new D{callback, results});
        }
    }).detach();
}

std::vector<SearchResult> SearchEngine::search_bins(const std::string& term) {
    std::vector<SearchResult> results;
    std::string path_env = getenv("PATH");
    std::stringstream ss(path_env);
    std::string dir;
    int limit = 20;

    while (std::getline(ss, dir, ':') && results.size() < limit) {
        if (!fs::exists(dir)) continue;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (results.size() >= limit) break;
            std::string name = entry.path().filename().string();
            // If term is empty, listing all bins is chaotic, so only search if term exists
            if (!term.empty() && Utils::contains_ignore_case(name, term)) {
                if (Utils::is_executable(entry.path())) {
                    results.push_back({
                        name,
                        entry.path().string(),
                        "utilities-terminal", 
                        entry.path().string(),
                        "bin"
                    });
                }
            }
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::search_clipboard(const std::string& term) {
    std::string output = Utils::exec("cliphist list");
    std::vector<SearchResult> results;
    std::stringstream ss(output);
    std::string line;
    
    while(std::getline(ss, line)) {
        size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;
        std::string id = line.substr(0, tab);
        std::string content = line.substr(tab + 1);

        if (term.empty() || Utils::contains_ignore_case(content, term)) {
            results.push_back({
                content.substr(0, 100), 
                "Cliphist ID: " + id,
                "edit-paste",
                "cliphist decode " + id + " | wl-copy",
                "clipboard"
            });
        }
    }

    if (results.empty()) {
        results.push_back({"No clipboard history found", "Install cliphist", "dialog-error", "", "info"});
    }

    return results;
}

std::vector<SearchResult> SearchEngine::search_calc(const std::string& term) {
    std::string sanitized;
    for(char c : term) {
        if (isalnum(c) || std::string("+-*/^(). ").find(c) != std::string::npos) {
            sanitized += c;
        }
    }

    if (sanitized.empty()) return {};
    std::string cmd = "echo '" + sanitized + "' | bc -l";
    std::string result = Utils::exec(cmd.c_str());
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return {{ result, "Click to copy result", "accessories-calculator", "echo -n '" + result + "' | wl-copy", "calc" }};
}

void SearchEngine::load_emojis() {
    if (emoji_loaded) return;
    std::ifstream f("emoji.json");
    if (f.good()) {
        try {
            f >> emoji_cache;
            emoji_loaded = true;
        } catch (...) {
            std::cerr << "Failed to parse emoji.json" << std::endl;
        }
    }
}

std::vector<SearchResult> SearchEngine::search_emoji(const std::string& term) {
    load_emojis();
    std::vector<SearchResult> results;
    if (!emoji_loaded) return {{"Error", "emoji.json not found", "dialog-error", "", "error"}};
    for (const auto& item : emoji_cache) {
        std::string text = item["text"];
        std::string emoji = item["emoji"];
        std::vector<std::string> keywords = item["keywords"];
        bool match = Utils::contains_ignore_case(text, term);
        if (!match) {
            for(const auto& k : keywords) {
                if(Utils::contains_ignore_case(k, term)) { match = true; break; }
            }
        }
        if (match) {
            results.push_back({
                emoji + "  " + text,
                item["category"],
                "", 
                "echo -n '" + emoji + "' | wl-copy",
                "emoji"
            });

            if (results.size() > 50) break;
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::search_cmd(const std::string& term) { 
    return {{ term, "Run command", "utilities-terminal", term, "cmd" }};
}

std::vector<SearchResult> SearchEngine::search_google_sync(const std::string& term) {

    std::string display = term.empty() ? "Type to search Google..." : term;
    return {{
        display,
        "Search Google (Enter to open)",
        "web-browser",
        "xdg-open \"https://www.google.com/search?q=" + term + "\"",
        "google",
        1000
    }};
}

std::vector<SearchResult> SearchEngine::search_youtube_sync(const std::string& term) {

    std::string display = term.empty() ? "Type to search YouTube..." : term;
    return {{
        display,
        "Search YouTube (Enter to open)",
        "multimedia-video-player",
        "xdg-open \"https://www.youtube.com/results?search_query=" + term + "\"",
        "youtube",
        1000
    }};

}

std::vector<SearchResult> SearchEngine::search_url(const std::string& term) {
    return {{ term, "Open URL", "network-server", "xdg-open \"" + term + "\"", "url" }};
}