#pragma once

#include "config.hpp"
#include "manager.hpp"

#include "../../helpers/config_parse.hpp"
#include "../../helpers/fs.hpp"
#include "../../helpers/logger.hpp"
#include "../../helpers/string.hpp"

#include <toml++/toml.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>

namespace Lawnch::Core::Config {

using Lawnch::Config::Color;
using Lawnch::Config::Padding;

inline std::string
resolveVar(const std::string &val,
           const std::map<std::string, Color> &theme_colors) {
  if (val.empty() || val[0] != '$')
    return val;

  std::string var_name = val.substr(1);
  auto it = theme_colors.find(var_name);
  if (it != theme_colors.end()) {
    auto toHex = [](double v) -> int {
      return static_cast<int>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
    };
    char buf[10];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", toHex(it->second.r),
                  toHex(it->second.g), toHex(it->second.b),
                  toHex(it->second.a));
    return std::string(buf);
  }

  Logger::log("Config", Logger::LogLevel::WARNING,
              "Unknown theme variable: " + val);
  return val;
}

inline Color readColor(const toml::node &node,
                       const std::map<std::string, Color> &theme_colors) {
  if (auto str = node.as_string()) {
    std::string val = str->get();
    val = resolveVar(val, theme_colors);
    return Lawnch::Config::parseColor(val);
  }
  return {0, 0, 0, 1};
}

inline Color getColor(const toml::table &tbl, std::string_view key,
                      const std::map<std::string, Color> &theme_colors,
                      const Color &def) {
  if (auto node = tbl[key]; node)
    return readColor(*node.node(), theme_colors);
  return def;
}

inline Padding getPadding(const toml::table &tbl, std::string_view key,
                          const Padding &def) {
  auto node = tbl[key];
  if (!node)
    return def;
  if (auto arr = node.as_array()) {
    std::vector<int64_t> vals;
    for (auto &el : *arr)
      if (auto v = el.as_integer())
        vals.push_back(v->get());
    return Lawnch::Config::parsePaddingFromArray(vals);
  }
  if (auto val = node.as_integer())
    return Padding(static_cast<int>(val->get()));
  if (auto str = node.as_string())
    return Lawnch::Config::parsePadding(str->get());
  return def;
}

inline std::string getStr(const toml::table &tbl, std::string_view key,
                          const std::string &def) {
  if (auto v = tbl[key].as_string())
    return v->get();
  return def;
}

inline int getInt(const toml::table &tbl, std::string_view key, int def) {
  if (auto v = tbl[key].as_integer())
    return static_cast<int>(v->get());
  return def;
}

inline double getDouble(const toml::table &tbl, std::string_view key,
                        double def) {
  if (auto v = tbl[key].as_floating_point())
    return v->get();
  if (auto v = tbl[key].as_integer())
    return static_cast<double>(v->get());
  return def;
}

inline bool getBool(const toml::table &tbl, std::string_view key, bool def) {
  if (auto v = tbl[key].as_boolean())
    return v->get();
  return def;
}

inline std::vector<std::string>
getStrArray(const toml::table &tbl, std::string_view key,
            const std::vector<std::string> &def) {
  auto node = tbl[key];
  if (!node)
    return def;
  if (auto arr = node.as_array()) {
    std::vector<std::string> result;
    for (auto &el : *arr)
      if (auto s = el.as_string())
        result.push_back(s->get());
    return result;
  }
  return def;
}

inline const toml::table *getTable(const toml::table &root,
                                   std::string_view path) {
  auto node = root.at_path(path);
  if (node && node.is_table())
    return node.as_table();
  return nullptr;
}

inline std::filesystem::path findDataFile(const std::string &subdir,
                                          const std::string &name,
                                          const std::string &config_path) {
  if (name.empty())
    return {};

  std::string filename = name + ".toml";

  for (const auto &dir : Lawnch::Fs::get_data_dirs()) {
    auto candidate = std::filesystem::path(dir) / "lawnch" / subdir / filename;
    if (std::filesystem::exists(candidate))
      return candidate;
  }

  auto candidate =
      std::filesystem::path(config_path).parent_path() / subdir / filename;
  if (std::filesystem::exists(candidate))
    return candidate;

  return {};
}

struct Manager::Impl {
  Config config;
  mutable std::shared_mutex config_mutex;

  void SetDefaults();
  void LoadThemeColors(const toml::table &theme_tbl);
  bool LoadThemeFile(const std::string &name, const std::string &config_path);
  void LoadPresetFile(const std::string &name, const std::string &config_path);

  void ApplyToml(const toml::table &root);

  void ApplyGeneral(const toml::table &root);
  void ApplyAppearance(const toml::table &root);
  void ApplyLaunch(const toml::table &root);
  void ApplyKeybindings(const toml::table &root);
  void ApplyWindow(const toml::table &root);
  void ApplyInput(const toml::table &root);
  void ApplyInputPrompt(const toml::table &root);
  void ApplyResults(const toml::table &root);
  void ApplyResultItem(const toml::table &root);
  void ApplyResultsCount(const toml::table &root);
  void ApplyPreview(const toml::table &root);
  void ApplyClock(const toml::table &root);
  void ApplyProviders(const toml::table &root);
  void ApplyPlugins(const toml::table &root);
};

} // namespace Lawnch::Core::Config
