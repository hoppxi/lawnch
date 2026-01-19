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

    mutable = mkOption {
      type = types.bool;
      default = false;
      description = "option to enable mustable config file";
    };

    settings = {
      general = mkOption {
        type = types.attrs;
        default = { };
        description = "General settings";
      };

      launch = mkOption {
        type = types.attrs;
        default = { };
        description = "Lauching options";
      };

      layout = mkOption {
        type = types.attrs;
        default = { };
        description = "Layout options";
      };

      window = mkOption {
        type = types.attrs;
        default = { };
        description = "Base window options";
      };

      input = mkOption {
        type = types.attrs;
        default = { };
        description = "Input box options";
      };

      input_prompt = mkOption {
        type = types.attrs;
        default = { };
        description = "Input prompt options";
      };

      results = mkOption {
        type = types.attrs;
        default = { };
        description = "Results container options";
      };

      result_item = mkOption {
        type = types.attrs;
        default = { };
        description = "Result item options";
      };

      preview = mkOption {
        type = types.attrs;
        default = { };
        description = "Icon preview options";
      };

      results_count = mkOption {
        type = types.attrs;
        default = { };
        description = "Results counter option";
      };

      clock = mkOption {
        type = types.attrs;
        default = { };
        description = "Clock component options";
      };
    };

    menus = mkOption {
      type = types.attrsOf types.attrs;
      default = { };
      description = "Extra menu configurations. Each attribute name will be the filename (param.ini) and value is the INI configuration.";
      example = literalExpression ''
        {
          powermenu = {
            input.visible = false;
            launch.start_with = ":p";
          };
        }
      '';
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
