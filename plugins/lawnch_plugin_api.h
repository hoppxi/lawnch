#ifndef LAWNCH_PLUGIN_API_H
#define LAWNCH_PLUGIN_API_H

#define PLUGIN_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

#define LAWNCH_PLUGIN_API_VERSION 4

typedef struct {
  char *name;
  char *comment;
  char *icon;
  char *command;
  char *type;
} LawnchResult;

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
} LawnchPluginVTable;

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void);

#ifdef __cplusplus
}
#endif

#endif // LAWNCH_PLUGIN_API_H
