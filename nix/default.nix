{
  pkgs,
  lawnch,
  lawnch-unwrapped,
  lawnch-plugin-api,
  knownPlugins ? { },
  ...
}:
{ config, lib, ... }:

with lib;

let
  cfg = config.programs.lawnch;
  toIni = pkgs.formats.ini { };

  generateFullConfig =
    let
      enabledPlugins = filterAttrs (_: v: v.enable) cfg.plugins;
      pluginSections = mapAttrs' (
        name: pluginCfg: nameValuePair "plugin/${name}" pluginCfg.settings
      ) enabledPlugins;
      enabledPluginsList = {
        plugins = mapAttrs (_: _: true) enabledPlugins;
      };
      finalSettings = cfg.settings // pluginSections // enabledPluginsList;
    in
    toIni.generate "config.ini" finalSettings;

  menuSources = mapAttrs (name: value: toIni.generate "${name}.ini" value) cfg.menus;

  collectIniFiles =
    dir:
    let
      entries = builtins.readDir dir;
    in
    builtins.concatLists (
      map (
        name:
        let
          path = dir + "/${name}";
          type = entries.${name};
        in
        if type == "regular" && builtins.match ".*\\.ini" name != null then
          [ path ]
        else if type == "directory" then
          collectIniFiles path
        else
          [ ]
      ) (builtins.attrNames entries)
    );

  themeIniFiles = collectIniFiles "${lawnch-unwrapped}/lib/lawnch/themes";

in
{
  imports = [ (import ./lawnch.nix { inherit lib lawnch; }) ];

  config = mkIf cfg.enable {

    xdg.configFile = mkMerge [
      { "lawnch/config.ini".source = generateFullConfig; }
      (mapAttrs' (name: source: nameValuePair "lawnch/${name}.ini" { inherit source; }) menuSources)
    ];

    home.packages =
      let
        buildPlugin =
          name: plugin:
          let
            sourcePath = if plugin.src != null then plugin.src else knownPlugins.${name};
          in
          pkgs.callPackage (sourcePath + "/default.nix") {
            inherit lawnch-plugin-api;
          };

        enabledPlugins = filterAttrs (_: v: v.enable) cfg.plugins;

        wrapPluginForProfile =
          name: plugin:
          let
            builtPkg = buildPlugin name plugin;
          in
          pkgs.runCommand "lawnch-plugin-${name}-profile" { } ''
            mkdir -p $out/share/lawnch/plugins/${name}
            cp ${builtPkg}/lib/lawnch/plugins/${name}.so $out/share/lawnch/plugins/${name}/

            if [ -f ${builtPkg}/lib/lawnch/plugins/pinfo/${name} ]; then
              cp ${builtPkg}/lib/lawnch/plugins/pinfo/${name} $out/share/lawnch/plugins/${name}/pinfo
            fi

            if [ -d ${builtPkg}/lib/lawnch/plugins/assets ]; then
              mkdir -p $out/share/lawnch/plugins/${name}/assets
              cp -r ${builtPkg}/lib/lawnch/plugins/assets/* $out/share/lawnch/plugins/${name}/assets/
            fi
          '';

        pluginPackages = mapAttrsToList wrapPluginForProfile enabledPlugins;

        themesPackage = pkgs.runCommand "lawnch-themes-profile" { } ''
          mkdir -p $out/share/lawnch/themes
          ${concatStringsSep "\n" (map (file: "cp ${file} $out/share/lawnch/themes/") themeIniFiles)}
        '';
      in
      [
        cfg.package
        themesPackage
      ]
      ++ pluginPackages;
  };
}
