#ifndef LAWNCH_PLUGIN_API_H
#define LAWNCH_PLUGIN_API_H

#include <stddef.h>
#include <stdint.h>

#define PLUGIN_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

#define LAWNCH_PLUGIN_API_VERSION 5

typedef struct {
  char *name;
  char *comment;
  char *icon;
  char *command;
  char *type;
  char *preview_image_path;
} LawnchResult;

// Flags for LawnchPluginVTable
// 1 << 0 => Disable history tracking for this plugin
#define LAWNCH_PLUGIN_FLAG_DISABLE_HISTORY (1 << 0)
// 1 << 1 => Disable sorting for this plugin (respects original order)
#define LAWNCH_PLUGIN_FLAG_DISABLE_SORT (1 << 1)

// Logging API
enum LawnchLogLevel {
  LAWNCH_LOG_CRITICAL,
  LAWNCH_LOG_ERROR,
  LAWNCH_LOG_WARNING,
  LAWNCH_LOG_INFO,
  LAWNCH_LOG_DEBUG
};

typedef struct {
  void (*log)(const char *logger_name, enum LawnchLogLevel level,
              const char *message);
} LawnchLogApi;

// Filesystem Helper API
typedef struct {
  // Path getters and returned strings must be freed with free_path
  char *(*get_home_path)(void);
  char *(*expand_tilde)(const char *path);
  char *(*get_config_home)(void);
  char *(*get_data_home)(void);
  char *(*get_cache_home)(void);
  char *(*get_log_path)(const char *app_name);
  char *(*get_socket_path)(const char *filename);

  // List getters and returned arrays must be freed with free_str_array, not the
  // strings inside individually
  char **(*get_data_dirs)(int *count);
  char **(*get_icon_dirs)(int *count);

  void (*free_path)(char *path);
  void (*free_str_array)(char **array, int count);
} LawnchFsApi;

// String Helper API
typedef struct {
  char *(*trim)(const char *str);
  char *(*to_lower_copy)(const char *str);
  char *(*unescape)(const char *str);
  char *(*escape)(const char *str);
  char *(*replace_all)(const char *str, const char *from, const char *to);

  // Tokenize
  char **(*tokenize)(const char *str, char delimiter, int *count);

  int (*iequals)(const char *a, const char *b);                 // bool 1/0
  int (*contains_ic)(const char *haystack, const char *needle); // bool 1/0
  int (*match_score)(const char *input, const char *target);
  size_t (*hash)(const char *str);

  void (*free_str)(char *str);
  void (*free_str_array)(char **array, int count);
} LawnchStrApi;

struct LawnchHostApi_s;

// A struct passed from the host to the plugin on init.
// It provides callbacks for the plugin to interact with the host.
typedef struct LawnchHostApi_s {
  int host_api_version;
  void *userdata; // Opaque pointer for the host's implementation

  // Gets a config value from the plugin's section in the main config.ini
  const char *(*get_config_value)(const struct LawnchHostApi_s *host,
                                  const char *key);

  // Gets the path to the application's shared data directory
  const char *(*get_data_dir)(const struct LawnchHostApi_s *host);

  const LawnchLogApi *log_api;
  const LawnchFsApi *fs_api;
  const LawnchStrApi *str_api;
} LawnchHostApi;

// Struct containing pointers to the plugin's functions
typedef struct {
  int plugin_api_version;

  void (*init)(const LawnchHostApi *host);
  void (*destroy)(void);
  const char **(*get_triggers)(void);
  LawnchResult *(*get_help)(
      void); // Returns a single result, wrapped as a pointer
  LawnchResult *(*query)(const char *term, int *num_results);
  void (*free_results)(LawnchResult *results, int num_results);

  uint32_t flags;
} LawnchPluginVTable;

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void);

#ifdef __cplusplus
}
#endif

#endif // LAWNCH_PLUGIN_API_H
