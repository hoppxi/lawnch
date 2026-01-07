{ lib, lawnch, ... }:

with lib;

{
  options.programs.lawnch = {
    enable = mkEnableOption "lawnch";

    package = mkOption {
      type = types.package;
      default = lawnch;
      description = "lawnch package to use.";
    };

    settings = {
      window = {
        width = mkOption {
          type = types.int;
          default = 500;
        };
        height = mkOption {
          type = types.int;
          default = 400;
        };
        anchor = mkOption {
          type = types.str;
          default = "top,center";
        };
        margin_top = mkOption {
          type = types.int;
          default = 120;
        };
        margin_bottom = mkOption {
          type = types.int;
          default = 50;
        };
        margin_left = mkOption {
          type = types.int;
          default = 50;
        };
        margin_right = mkOption {
          type = types.int;
          default = 50;
        };
        background_color = mkOption {
          type = types.str;
          default = "rgba(40, 40, 40, 0.80)";
        };
        border_color = mkOption {
          type = types.str;
          default = "rgba(69, 133, 136, 0.80)";
        };
        border_radius = mkOption {
          type = types.int;
          default = 12;
        };
        border_width = mkOption {
          type = types.int;
          default = 2;
        };
      };

      input = {
        font_family = mkOption {
          type = types.str;
          default = "Inter";
        };
        font_size = mkOption {
          type = types.int;
          default = 10;
        };
        font_weight = mkOption {
          type = types.str;
          default = "bold";
        };
        text_color = mkOption {
          type = types.str;
          default = "rgba(235, 219, 178, 1)";
        };
        placeholder_color = mkOption {
          type = types.str;
          default = "rgba(153, 144, 120, 1)";
        };
        background_color = mkOption {
          type = types.str;
          default = "rgba(40, 40, 40, 0.80)";
        };
        caret_color = mkOption {
          type = types.str;
          default = "rgba(153, 144, 120, 1)";
        };
        border_radius = mkOption {
          type = types.int;
          default = 8;
        };
        border_width = mkOption {
          type = types.int;
          default = 1;
        };
        border_color = mkOption {
          type = types.str;
          default = "rgba(108, 112, 134, 1)";
        };
        padding_top = mkOption {
          type = types.int;
          default = 10;
        };
        padding_bottom = mkOption {
          type = types.int;
          default = 10;
        };
        padding_left = mkOption {
          type = types.int;
          default = 10;
        };
        padding_right = mkOption {
          type = types.int;
          default = 10;
        };
        horizontal_align = mkOption {
          type = types.enum [
            "fill"
            "left"
            "center"
            "right"
          ];
          default = "fill";
        };
      };

      results = {
        font_family = mkOption {
          type = types.str;
          default = "Inter";
        };
        font_size = mkOption {
          type = types.int;
          default = 10;
        };
        item_spacing = mkOption {
          type = types.int;
          default = 2;
        };
        item_padding = mkOption {
          type = types.int;
          default = 8;
        };
        default_text_color = mkOption {
          type = types.str;
          default = "rgba(69, 133, 136, 1)";
        };
        default_background_color = mkOption {
          type = types.str;
          default = "rgba(0, 0, 0, 0)";
        };
        selected_text_color = mkOption {
          type = types.str;
          default = "rgba(164, 250, 255, 0.8)";
        };
        selected_background_color = mkOption {
          type = types.str;
          default = "rgba(69, 133, 136, 0.8)";
        };
        default_border_radius = mkOption {
          type = types.int;
          default = 8;
        };
        default_border_width = mkOption {
          type = types.int;
          default = 0;
        };
        default_border_color = mkOption {
          type = types.str;
          default = "rgba(0, 0, 0, 0.8)";
        };
        selected_border_radius = mkOption {
          type = types.int;
          default = 6;
        };
        selected_border_width = mkOption {
          type = types.int;
          default = 6;
        };
        selected_border_color = mkOption {
          type = types.str;
          default = "rgba(0, 0, 0, 0.8)";
        };
        enable_comment = mkOption {
          type = types.bool;
          default = true;
        };
        comment_font_size = mkOption {
          type = types.int;
          default = 8;
        };
        comment_color = mkOption {
          type = types.str;
          default = "rgba(104, 157, 106, 1)";
        };
        selected_comment_color = mkOption {
          type = types.str;
          default = "rgba(157, 237, 160, 1)";
        };
        enable_icon = mkOption {
          type = types.bool;
          default = true;
        };
        icon_size = mkOption {
          type = types.int;
          default = 24;
        };
      };
    };

    plugins = mkOption {
      type = types.attrsOf (
        types.submodule (
          { ... }:
          {
            options = {
              enable = mkEnableOption "plugin";

              version = mkOption {
                type = types.str;
                default = "0.1.0";
                description = "Version of the plugin.";
              };

              src = mkOption {
                type = types.nullOr (
                  types.oneOf [
                    types.package
                    types.path
                    types.str
                  ]
                );
                default = null;
                description = "Path to plugin source. If null, tries to find it in default ./plugins dir. Accepts local paths, derivations (pkgs.fetchFromGitHub), or flake inputs.";
              };

              settings = mkOption {
                type = types.attrs;
                default = { };
                description = "Plugin specific settings.";
              };

              extraBuildInputs = mkOption {
                type = types.listOf types.package;
                default = [ ];
                description = "Extra build inputs for the plugin.";
              };
            };
          }
        )
      );
      default = { };
      description = "Enable and configure plugins.";
    };
  };
}
