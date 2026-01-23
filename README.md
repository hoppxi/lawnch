# Lawnch

It is a lightweight and featureful alternative launcher for wayland. It is built with Blend2D, which is cpu-accelerated drawing library, letting the launcher to be small.

## Features

- It has many plugins. and plugins can be switched even after launching the launcher with vim like key commands
- It is customizable with lots of components like preview included
- It has builtin plugin manager with nixos supported well with home-manager
- It has builtin theme to play around without you having to do any config
- It is lightweight and very fast by loading only the needed part of the app or plugin with lazy loading

## Usage

It uses a filter command system kind of like vim. You can switch to specific plugins (context) by typing a colon followed by a keyword.

| Command          | Action                                                |
| ---------------- | ----------------------------------------------------- |
| `:h` or `:help`  | Show all available plugins and their filter keywords. |
| `:a` or `:apps`  | Search installed applications.                        |
| `:b` or `:bins`  | Search executables in your `$PATH`.                   |
| `:<plugin_name>` | Switch to the the plugin (e.g. :e for emoji).         |

> Note: Filters are defined by the plugins themselves and can be customized. plugins can allow to customize filter key.

## Installation

### Nixos

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
      enable = true; # installs wallpapers plugin and configure the plugin
      settings = {
        dir = "~/Pictures/Wallpapers";
        command = "waul --set '{}'";
      };
    };
  };

  menus = {
    # create menus that will be symlinked to the same dir to the config.ini
    # by running `lawnch -m ~/.config/lawnch/powermenu.ini you can merge this setting with the main
    # config and override some properties for the menu like in here for powermenu to not show input and
    # to limit the window to the powermenu plugin context
    powermenu = {
      window.anchor = "center,center";
      launch.context = ":p";
      results_count.enable = false;
      layout.order = "results";
      input.visible = false;
    };
  };
};

```

### Other distro

Requirements:

- `cmake`, `gcc`, `pkg-config`
- `wayland`, `wayland-scanner`, `wlr-protocols`
- `blend2d`, `inih`, `libxkbcommon`, `nanosvg`
- `fontconfig`, `libffi`, `expat`

```bash
git clone https://github.com/hoppxi/lawnch.git
cd lawnch
cmake -B build
cmake --build build
```

Manage Plugins:

```bash
lawnch pm install ./plugins/clipboard
lawnc  pm enable clipboard
lawnch pm list
```

## Configuration

Configuration loaded from `~/.config/lawnch/config.ini`.

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

[plugins]
powermenu=true
wallpapers=true
calculator=true

# Plugin specific settings
[plugin/wallpapers]
dir = ~/Pictures/Wallpapers
max_results = 10

```

you can find a complete config example at config/config.ini. Also you do not need config to run the app.

## Plugin

It is simple to build plugins for lawnch, with the api you can do anything including query since it gives you the power to query whatever is entered to the input.

It allows you to get config keys from the main app's config with also the ability to access and create assets to work with. see the powermenu pllugin for a config loading example and emoji to see how assets can be helpful.

## Known issues

- currently, lazy loading images and disk caching them for the first time does take a bit of RAM and CPU. Once loaded you won't see spikes and will be resolved

## Contributing

If you are interested contribute to the plugins, themes or anything you can.

## License

MIT
