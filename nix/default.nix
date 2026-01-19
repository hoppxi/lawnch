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

  configDir = "lawnch";
  fullConfigPath = "${config.xdg.configHome}/${configDir}";

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

  themeLinks = builtins.listToAttrs (
    map (
      file:
      let
        base = builtins.baseNameOf file;
      in
      {
        name = "lawnch/themes/${builtins.unsafeDiscardStringContext base}";
        value = {
          source = file;
        };
      }
    ) themeIniFiles
  );

in
{
  imports = [ (import ./lawnch.nix { inherit lib lawnch; }) ];

  config = mkIf cfg.enable {
    home.packages = [ cfg.package ];

    xdg.configFile = mkMerge [
      (mkIf (!cfg.mutable) (mkMerge [
        { "lawnch/config.ini".source = generateFullConfig; }
        (mapAttrs' (name: source: nameValuePair "lawnch/${name}.ini" { inherit source; }) menuSources)
      ]))
    ];

    home.activation.backupLawnchConfigs = mkIf (!cfg.mutable) (
      lib.hm.dag.entryBefore [ "checkLinkTargets" ] ''
        backup_if_real() {
          local f="$1"
          if [ -e "$f" ] && [ ! -L "$f" ]; then
            local i=1
            while [ -e "$f.b$i" ]; do i=$((i+1)); done
            $DRY_RUN_CMD mv "$f" "$f.b$i"
          fi
        }
        backup_if_real "${fullConfigPath}/config.ini"
        ${concatStringsSep "\n" (
          map (name: ''
            backup_if_real "${fullConfigPath}/${name}.ini"
          '') (builtins.attrNames cfg.menus)
        )}
      ''
    );

    home.activation.setupLawnchConfig = mkIf cfg.mutable (
      lib.hm.dag.entryAfter [ "writeBoundary" ] ''
        $DRY_RUN_CMD mkdir -p "${fullConfigPath}"

        setup_mutable() {
          local src="$1"
          local dest="$2"
          if [ ! -f "$dest" ]; then
            $DRY_RUN_CMD cp -f "$src" "$dest"
            $DRY_RUN_CMD chmod 600 "$dest"
          fi
        }

        setup_mutable "${generateFullConfig}" "${fullConfigPath}/config.ini"
        ${concatStringsSep "\n" (
          mapAttrsToList (name: source: ''
            setup_mutable "${source}" "${fullConfigPath}/${name}.ini"
          '') menuSources
        )}
      ''
    );

    xdg.dataFile =
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
        enabledPluginPkgs = mapAttrsToList (name: plugin: buildPlugin name plugin) enabledPlugins;

        mergedAssets = pkgs.runCommand "lawnch-merged-assets" { } ''
          mkdir -p $out
          ${concatStringsSep "\n" (
            map (pkg: ''
              if [ -d "${pkg}/lib/lawnch/assets" ]; then
                cp -rsnf ${pkg}/lib/lawnch/assets/. $out/
                chmod -R +w $out/ || true
              fi
            '') enabledPluginPkgs
          )}
        '';
      in
      mkMerge (
        [
          themeLinks
          {
            "lawnch/assets" = {
              source = mergedAssets;
              recursive = true;
            };
          }
        ]
        ++ (mapAttrsToList (
          name: plugin:
          let
            builtPkg = buildPlugin name plugin;
          in
          {
            "lawnch/plugins/${name}.so" = {
              source = "${builtPkg}/lib/lawnch/plugins/${name}.so";
              executable = true;
            };
            "lawnch/plugins/pinfo/${name}" = {
              source = "${builtPkg}/lib/lawnch/plugins/pinfo/${name}";
            };
          }
        ) enabledPlugins)
      );
  };
}
