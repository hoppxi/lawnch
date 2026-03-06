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

    settings = mkOption {
      type = types.attrs;
      default = { };
      description = "Config keys (converted to TOML format).";
    };

    menus = mkOption {
      type = types.attrsOf types.attrs;
      default = { };
      description = "Extra menu configurations. Each key becomes a <name>.toml file.";
      example = literalExpression ''
        {
          powermenu = {
            widget.input.visible = false;
            launch.scope = ":p";
          };
        }
      '';
    };

    plugins = mkOption {
      type = types.attrsOf (
        types.submodule (
          { name, ... }:
          {
            options = {
              enable = mkEnableOption "plugin ${name}";

              package = mkOption {
                type = types.nullOr types.package;
                default = null;
                description = "Plugin package. If null, uses official registry.";
              };

              settings = mkOption {
                type = types.attrs;
                default = { };
                description = "Plugin specific settings added to config.toml.";
              };
            };
          }
        )
      );
      default = { };
      description = "Configure and enable plugins.";
    };
  };
}
