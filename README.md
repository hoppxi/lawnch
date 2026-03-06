# Lawnch

It is a lightweight and featureful alternative launcher for wayland. It is built with Blend2D, which is cpu-accelerated drawing library, letting the launcher to be small.

> [!WARNING]
> **This project is in early stage (v0.3.0-alpha).** Many features are untested, incomplete, or may crash. The configuration format, plugin API, technology stack, and overall architecture are all subject to drastic changes without notice between versions. Use at your own risk, until we reach stable version.

## Features

- customizable with lots of components like preview included
- support for submenu section like in apps for desktop action section and for file navigation
- support for uwsm managed session
- many plugins those can be switched even after launching the launcher with vim like key commands
- builtin plugin manager with nixos supported well including home-manager
- builtin theme and presets to play around without you having to do any config
- lightweight and very fast by loading only the needed part of the app or plugin with lazy loading

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

### NixOS / Home Manager

Lawnch has a flake with both a NixOS module and a Home Manager module.

```nix
inputs = {
  lawnch = {
    url = "github:hoppxi/lawnch";
    inputs.follows.nixpkgs = "nixpkgs";
  };
  # ...
};
```

```nix
imports = [
  inputs.lawnch.homeModules.default # or inputs.lawnch.nixosModules.default if using nixosConfigurations
];

programs.lawnch = {
  enable = true;
  package = inputs.lawnch.packages.${pkgs.system}.default;
  settings = {
    window.width = 600;
    input.font.size = 12;
  };
  plugins = {
    wallpapers = {
      enable = true;
      settings = {
        dir = "~/Pictures/Wallpapers";
        command = "awww --set '{}'";
      };
    };
  };
  menus = {
    powermenu = {
      widget.input.visible = false;
      launch.scope = ":p";
    };
  };
};
```

### Other distro

Requirements:

- `cmake`, `gcc`, `pkg-config`
- `wayland`, `wayland-scanner`, `wlr-protocols`
- `blend2d`, `inih`, `tomlplusplus`, `libxkbcommon`, `nanosvg`
- `fontconfig`, `libffi`, `expat`

```bash
git clone https://github.com/hoppxi/lawnch.git
cd lawnch
cmake -B build
cmake --build build -j$(nproc)

cmake --install build # you might want to add sudo to install it system wide
```

## Configuration

Configuration loaded from `~/.config/lawnch/config.toml`.

```toml
[window]
width           = 500
height          = 410
anchor          = "top, center"
margin          = [140, 0, 0, 0]

[widget.input]
font            = { family = "JetBrainsMono NerdFont", size = 15, weight = "normal" }
text            = "#EBDBB2"

[widget.input.prompt]
enable          = true
text            = " >>"

[plugin.powermenu]
enable          = false
order           = ["lockscreen", "shutdown", "reboot", "suspend", "hibernate"]
```

you can find a complete config example at [config/config.toml](https://github.com/hoppxi/lawnch/tree/main/config/config.toml). Lawnch has default configuration built in so you do not need config file to start the app and try.

## Plugin

Plugins are found at [https://github.com/hoppxi/lawnch-plugins](https://github.com/hoppxi/lawnch-plugins). Installing plugins is done using the buitin command `lawnch pm` like this:

```bash
lawnch pm install https://github.com/hoppxi/lawnch-plugins/{plugin-name}

lawnch pm enable {plugin-name} # this will not work if you are on nixos and using symlinked config. instead, enable it in your config

# list of plugins is also found in the repo, check it
```

It is simple to build plugins for lawnch, with the api you can do anything including query since it gives you the power to query whatever is entered to the input.

It allows you to get config keys from the main app's config with also the ability to access and create assets to work with. see the powermenu pllugin for a config loading example and emoji to see how assets can be helpful.

## Known issues

- currently, lazy loading images and disk caching them for the first time does take a bit of RAM and CPU. Once loaded you won't see spikes and will be resolved

## Contributing

If you are interested contribute to the plugins, themes or anything you can.

## License

MIT
