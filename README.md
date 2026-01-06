# Lawnch

lightweight and strictly Wayland-native launcher. Built with wlr and cairo to fast and easy on RAM by avoiding heavy toolkits like GTK.

It features a plugin system and Vim-like mode filtering.

## Features

- Pure Wayland: Built using `wayland-client` and `wlr-layer-shell` protocols.
- Zero Bloat: No GTK/Qt dependencies. Just, Cairo, and Pango.
- Vim-like Filters: Use commands like `:h` or `:apps` to switch between search contexts.
- Plugin System: Extensible via shared objects (`.so`). Write your own or use plugins from here.
- `lawnchpm`: A built-in plugin manager to build and install plugins on non-Nix distros.

## Usage

Lawnch uses a filter command system kind of like Vim. You can switch to specific plugins by typing a colon followed by a keyword.

| Command          | Action                                                |
| ---------------- | ----------------------------------------------------- |
| `:h` or `:help`  | Show all available plugins and their filter keywords. |
| `:a` or `:apps`  | Search installed applications.                        |
| `:b` or `:bins`  | Search executables in your `$PATH`.                   |
| `:<plugin_name>` | Switch to the the plugin (e.g. :p for powermenu).     |

> Note: Filters are defined by the plugins themselves and can be customized. plugins can allow to customize filter key.

## Installation

### Nix / NixOS (Recommended)

Lawnch has a flake with a home manager module.

```nix
inputs = {
  lawnch.url = "github:hoppxi/lawnch";
  # ...
};

# in your config
imports = [
  inputs.lawnch.homeModules.default
];

programs.lawnch = {
  enable = true;
  package = inputs.lawnch.packages.${pkgs.system}.default;
  settings = {
    window.width = 600;
    input.font_size = 12;
  };
  plugins = {
    wallpapers = {
      enable = true;
      settings = {
        dir = "~/Pictures/Wallpapers";
        command = "waul --set '{}'";
      };
    };
  };
};

```

### Other distro

Requirements:

- `cmake`, `gcc`, `pkg-config`
- `wayland`, `wayland-scanner`, `wlr-protocols`
- `cairo`, `pango`, `xkbcommon`, `nlohmann_json`, `curl`.
- `inih`, `harfbuzz`, `librsvg`, `fontconfig`

```bash
git clone https://github.com/hoppxi/lawnch.git
cd lawnch
cmake -B build
cmake --build build
# copy to $PATH, currenlty the makefile does not define install
```

Manage Plugins with `lawnchpm`:

```bash
lawnchpm install ./plugins/clipboard
lawnchpm enable clipboard
lawnchpm list

```

## Configuration

Configuration loaded from `~/.config/lawnch/config.ini`.

### Styling

```ini
[window]
width=500
height=400
background_color=rgba(40, 40, 40, 0.80)
border_color=rgba(69, 133, 136, 0.80)
border_radius=12

[input]
font_family=Inter
font_size=10
text_color=rgba(235, 219, 178, 1)
horizontal_align=fill

[results]
selected_background_color=rgba(69, 133, 136, 0.8)
icon_size=24
```

you can find a complete config example at config/config.ini. and also config is not mandatory.

### Plugins

To enable plugins manually in the config:

```ini
[plugins]
powermenu=true
wallpapers=true
calculator=true

# Plugin specific settings
[plugin/wallpapers]
dir = ~/Pictures/Wallpapers
max_results = 10

```

## Plugin System

Lawnch plugins are compiled shared libraries (`.so`).

- located: Plugins are stored in `~/.local/share/lawnch/plugins`.
- Development: Plugins uses the `lawnch_plugin_api.h` header.
- Structure: A plugin directory usually contains the C++ source and a `CMakeLists.txt` and `deps.nix` (if any deps needed to build).

To create a new plugin, check the `plugins/` directory. and see powermenu as example plugin implementing the `lawnch_plugin_api.h` .

## Contributing

If you are interested contribute to the plugins or anything you can.

## License

MIT
