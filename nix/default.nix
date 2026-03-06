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
  toTOML = pkgs.formats.toml { };
  isHm = platform == "home-manager";

  generateFullConfig =
    let
      enabledPlugins = filterAttrs (_: v: v.enable) cfg.plugins;

      pluginSections = {
        plugin = mapAttrs (
          name: pcfg: { enable = true; } // pcfg.settings
        ) enabledPlugins;
      };

      finalSettings = cfg.settings // pluginSections;
    in
    toTOML.generate "config.toml" finalSettings;

  menuSources = mapAttrs (name: val: toTOML.generate "${name}.toml" val) cfg.menus;

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
    { "lawnch/config.toml".source = generateFullConfig; }
    (mapAttrs' (name: src: nameValuePair "lawnch/${name}.toml" { source = src; }) menuSources)
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
