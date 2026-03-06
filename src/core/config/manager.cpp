#include "manager_impl.hpp"

namespace Lawnch::Core::Config {

Manager &Manager::Instance() {
  static Manager instance;
  return instance;
}

Manager::Manager() : m_impl(std::make_unique<Impl>()) {}
Manager::~Manager() = default;

bool Manager::Impl::LoadThemeFile(const std::string &name,
                                  const std::string &config_path) {
  auto file = findDataFile("themes", name, config_path);
  if (file.empty()) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Theme '" + name + "' not found in any data directory");
    return false;
  }
  try {
    auto tbl = toml::parse_file(file.string());
    LoadThemeColors(tbl);
    Logger::log("Config", Logger::LogLevel::INFO,
                "Loaded theme: " + name + " (" + file.string() + ")");
    return true;
  } catch (const toml::parse_error &e) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Failed to parse theme '" + name +
                    "': " + std::string(e.what()));
    return false;
  }
}

void Manager::Impl::LoadPresetFile(const std::string &name,
                                   const std::string &config_path) {
  auto file = findDataFile("presets", name, config_path);
  if (file.empty()) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Preset '" + name + "' not found in any data directory");
    return;
  }
  try {
    auto tbl = toml::parse_file(file.string());
    ApplyToml(tbl);
    Logger::log("Config", Logger::LogLevel::INFO,
                "Loaded preset: " + name + " (" + file.string() + ")");
  } catch (const toml::parse_error &e) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Failed to parse preset '" + name +
                    "': " + std::string(e.what()));
  }
}

void Manager::Load(const std::string &path) {
  std::unique_lock lock(m_impl->config_mutex);
  m_impl->SetDefaults();

  toml::table user_config;
  try {
    user_config = toml::parse_file(path);
  } catch (const toml::parse_error &e) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Unable to parse '" + path + "': " + std::string(e.what()) +
                    ". Using defaults.");
    return;
  }

  std::string theme_name, preset_name;
  if (auto *a = getTable(user_config, "appearance")) {
    theme_name = getStr(*a, "theme", "");
    preset_name = getStr(*a, "preset", "");
  }

  bool theme_ok = theme_name.empty() || m_impl->LoadThemeFile(theme_name, path);

  if (!preset_name.empty()) {
    if (!theme_ok)
      Logger::log("Config", Logger::LogLevel::WARNING,
                  "Skipping preset '" + preset_name + "' — theme '" +
                      theme_name + "' failed to load");
    else
      m_impl->LoadPresetFile(preset_name, path);
  }

  m_impl->ApplyToml(user_config);

  Logger::log("Config", Logger::LogLevel::INFO,
              "Configuration loaded from '" + path + "'");
}

void Manager::Merge(const std::string &path) {
  std::unique_lock lock(m_impl->config_mutex);
  try {
    auto tbl = toml::parse_file(path);
    m_impl->ApplyToml(tbl);
    Logger::log("Config", Logger::LogLevel::INFO,
                "Merged configuration from '" + path + "'");
  } catch (const toml::parse_error &e) {
    Logger::log("Config", Logger::LogLevel::WARNING,
                "Unable to merge '" + path + "': " + std::string(e.what()) +
                    ". Skipping.");
  }
}

const Config &Manager::Get() const {
  std::shared_lock lock(m_impl->config_mutex);
  return m_impl->config;
}

} // namespace Lawnch::Core::Config
