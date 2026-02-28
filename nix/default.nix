{
  pkgs,
  lawnch,
  available-plugins ? { },
  platform ? "home-manager",
  ...
}:
{
  config,
  lib,
  ...
}:

with lib;

let
  cfg = config.programs.lawnch;
  toIni = pkgs.formats.ini { };
  isHm = platform == "home-manager";

  generateFullConfig =
    let
      enabledPlugins = filterAttrs (_: v: v.enable) cfg.plugins;

      pluginSections = mapAttrs' (
        name: pcfg: nameValuePair "plugin/${name}" pcfg.settings
      ) enabledPlugins;

      enabledPluginsList = {
        plugins = mapAttrs (_: _: true) enabledPlugins;
      };

      finalSettings = cfg.settings // pluginSections // enabledPluginsList;
    in
    toIni.generate "config.ini" finalSettings;

  menuSources = mapAttrs (name: val: toIni.generate "${name}.ini" val) cfg.menus;

  pluginPackages =
    let
      enabledPlugins = filterAttrs (_: v: v.enable) cfg.plugins;
    in
    mapAttrsToList (
      name: pcfg:
      if pcfg.package != null then
        pcfg.package
      else if hasAttr name available-plugins then
        available-plugins.${name}
      else
        throw "Plugin '${name}' not found in official registry."
    ) enabledPlugins;

  allPackages = [
    cfg.package
    lawnch
  ]
  ++ pluginPackages;

  lawnchConfig = mkMerge [
    { "lawnch/config.ini".source = generateFullConfig; }
    (mapAttrs' (name: src: nameValuePair "lawnch/${name}.ini" { source = src; }) menuSources)
  ];

in
{
  imports = [
    (import ./options.nix { inherit lib lawnch; })
  ];

  config = mkIf cfg.enable (
    if isHm then
      {
        xdg.configFile = lawnchConfig;
        home.packages = allPackages;
      }
    else
      {
        environment.etc = mapAttrs' (name: value: nameValuePair "xdg/${name}" value) lawnchConfig;
        environment.systemPackages = allPackages;
      }
  );
}
