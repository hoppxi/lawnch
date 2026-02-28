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
      type = types.attrsOf types.attrs;
      default = { };
      description = "Config keys (converted to INI format).";
    };

    menus = mkOption {
      type = types.attrsOf types.attrs;
      default = { };
      description = "Extra menu configurations. Each key becomes a <name>.ini file.";
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
                description = "Plugin specific settings added to config.ini.";
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
