#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Lawnch::CLI {

struct PluginInfo {
  std::string name;
  std::string version;
  std::string author;
  std::string description;
  std::string url;
  std::string src_dir;
  std::vector<std::string> dependencies;
  std::vector<std::pair<std::string, std::string>> assets; // {source, dest}
};

class PluginManager {
public:
  static int handle_command(const std::vector<std::string> &args);
  static void print_help();

private:
  static PluginInfo parse_plugin_info(const std::string &path);
  static void validate_pinfo(const PluginInfo &info,
                             const std::string &base_path);
  static void build(const std::string &plugin_dir);
  static void install(const std::string &plugin_dir);
  static void install_from_url(const std::string &url);
  static void uninstall(const std::string &plugin_name);
  static void list(const std::string &filter);
  static void enable(const std::string &plugin_name);
  static void disable(const std::string &plugin_name);
  static void info(const std::string &plugin_name);

  static std::string resolve_asset_var(const std::string &str,
                                       const std::string &src_dir,
                                       const std::string &build_dir,
                                       const std::string &out_dir);
};

} // namespace Lawnch::CLI
