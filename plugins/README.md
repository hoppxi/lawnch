# Lawnch Plugins

This directory contains plugins for Lawnch.

## Plugins

- [Calculator](https://github.com/hoppxi/lawnch/tree/main/plugins/calculator/README.md) – Perform quick math operations directly in the search bar.
- [Clipboard](https://github.com/hoppxi/lawnch/tree/main/plugins/clipboard/README.md) – Manage and search through your clipboard history.
- [Command](https://github.com/hoppxi/lawnch/tree/main/plugins/command/README.md) – Execute system commands.
- [Emoji](https://github.com/hoppxi/lawnch/tree/main/plugins/emoji/README.md) – Search for emojis and copy them to your clipboard.
- [Files](https://github.com/hoppxi/lawnch/tree/main/plugins/files/README.md) – Fast file system search and indexing.
- [Google](https://github.com/hoppxi/lawnch/tree/main/plugins/google/README.md) – Web search via Google.
- [Power Menu](https://github.com/hoppxi/lawnch/tree/main/plugins/powermenu/README.md) – Ppower actions (Shutdown, Reboot, Lock, etc.).
- [URL](https://github.com/hoppxi/lawnch/tree/main/plugins/url/README.md) – Open web addresses.
- [Wallpapers](https://github.com/hoppxi/lawnch/tree/main/plugins/wallpapers/README.md) – Browse and set desktop wallpapers.
- [YouTube](https://github.com/hoppxi/lawnch/tree/main/plugins/youtube/README.md) – Search for videos and open them in your browser.

## Plugin Development

Lawnch plugins are written in C++ and compiled as shared libraries (`.so`). They communicate with the host application via a VTable API.

### 1. Plugin Structure

A plugin directory should look like this:

```txt
my_plugin/
├── my_plugin.cpp    # Plugin logic. you can have as many file as you want.
├── CMakeLists.txt   # Build configuration
├── deps.nix         # (Optional) Nix dependencies & assets
└── README.md        # Description
```

### 2. Implementation

Your plugin must include `lawnch_plugin_api.h` found at `plugins/` and implement the `LawnchPluginVTable`.

#### Functions to Implement:

- `init`: Called when the plugin is loaded. Use this to load configs or cache data.
- `get_triggers`: Returns an array of strings (e.g., `":p"`) that activate your plugin. It should have only two string the first being the long trigger and the seocnd being the alternative short trigger. (e.g. `{":power", ":p", nullptr}`)
- `query`: The main part of the plugin. It receives the user's input and returns an array of `LawnchResult` objects.
- `free_results`: Called by the host to safely clean up the memory allocated during a query.

#### The Entry Point:

Every plugin must export the following function:

```cpp
PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) {
    return &g_vtable;
}

```

### 3. Building and Dependencies

#### Using CMake

Your `CMakeLists.txt` should produce a shared library with no `lib` prefix.

```cmake
add_library(my_plugin SHARED my_plugin.cpp)
set_target_properties(my_plugin PROPERTIES PREFIX "")

# Installation path
install(TARGETS my_plugin DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins)

```

#### Handling assets and eependencies (`deps.nix`)

If your plugin requires external libraries (like `libcurl`, `nlohmann_json`) or static data files (like `assets.json`), define them in a `deps.nix` file:

```nix
{
  deps = [ "curl" ]; # Libraries needed at build time
  assets = [ "assets.json" ]; # Files copied to the app data directory
}

# if your plugin only have deps and no assets you can di
[ "curl" ]

```

In your code, access these assets by calling `host->get_data_dir(host)`.

### 4. How to Build & Run

1. Place your plugin in the `plugins/` directory.
2. Make sure the API header `lawnch_plugin_api.h` is accessible in the parent directory.
3. Compile and install like

```bash
cmake -B build
cmake --build build
sudo make install # if you wnat to install it.

```

_This installs the `.so` to `/usr/lib/lawnch/plugins/` and assets to `/usr/share/lawnch/assets/`. but you should copy the `.so` exec to `~/.local/share/lawnch/plugins` and your assets to `~/.local/share/lawnch/assets/`_

Always use `c_strdup` or a similar helper for strings returned in `LawnchResult`, as the host will attempt to free them. Keep `plugin_query` fast. If you need to do heavy lifting, do it during `init` or in a background thread. Use unique triggers to avoid conflicts with other plugins.
