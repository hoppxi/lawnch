{
  pkgs,
  lawnch,
  lawnch-unwrapped,
  knownPlugins ? { },
  ...
}:
{ config, lib, ... }:

with lib;

let
  cfg = config.programs.lawnch;

  getPluginInfo =
    name: pluginCfg:
    let
      sourcePath =
        if pluginCfg.src != null then
          pluginCfg.src
        else if builtins.hasAttr name knownPlugins then
          knownPlugins.${name}
        else
          throw "Lawnch: Plugin '${name}' is enabled but no 'src' was provided and it was not found in ./plugins";

      depFilePath = sourcePath + "/deps.nix";

      rawDeps =
        if builtins.pathExists depFilePath then
          import depFilePath
        else
          {
            deps = [ ];
            assets = [ ];
          };

    in
    {
      inherit sourcePath;
      assets = if builtins.isList rawDeps then [ ] else (rawDeps.assets or [ ]);
      deps = if builtins.isList rawDeps then rawDeps else (rawDeps.deps or [ ]);
    };

  # A function to build a self-contained plugin
  buildPlugin =
    name: pluginCfg:
    let
      info = getPluginInfo name pluginCfg;
      autoDeps = map (dep: pkgs.${dep}) info.deps;
    in
    pkgs.stdenv.mkDerivation {
      pname = "lawnch-plugin-${name}";
      version = pluginCfg.version;
      src = info.sourcePath;

      nativeBuildInputs = with pkgs; [
        cmake
        pkg-config
      ];

      buildInputs = with pkgs; [ lawnch-unwrapped ] ++ pluginCfg.extraBuildInputs ++ autoDeps;

      # cmakeFlags = [
      #   "-DLAWNCH_DATA_DIR=${lawnch-unwrapped}/share/lawnch"
      # ];

      preConfigure = ''
        cp ${lawnch-unwrapped}/include/lawnch/plugins/lawnch_plugin_api.h ../lawnch_plugin_api.h
      '';

      installPhase = ''
        runHook preInstall
        mkdir -p $out/lib/lawnch/plugins
        cp ${name}.so $out/lib/lawnch/plugins/

        ${builtins.concatStringsSep "\n" (
          map (asset: ''
            echo "Installing asset: ${asset}"
            mkdir -p "$out/lib/lawnch/assets"
            mkdir -p "$(dirname "$out/lib/lawnch/assets/${asset}")"
            cp -r "$src/${asset}" "$out/lib/lawnch/assets/${asset}"
          '') info.assets
        )}

        runHook postInstall
      '';
    };

  toIni = pkgs.formats.ini { };

  generateFullConfig =
    let
      enabledPlugins = filterAttrs (n: v: v.enable) cfg.plugins;

      # Creates sections like [plugin/plugin_name]
      pluginSections = mapAttrs' (
        name: pluginCfg: nameValuePair "plugin/${name}" pluginCfg.settings
      ) enabledPlugins;

      # Creates the [plugins] section with 'plugin_name = true'
      enabledPluginsList = {
        plugins = mapAttrs (_: _: true) enabledPlugins;
      };

      finalSettings = cfg.settings // pluginSections // enabledPluginsList;
    in
    (toIni.generate "config.ini" finalSettings);

in
{
  imports = [ (import ./lawnch.nix { inherit lib lawnch; }) ];

  config = mkIf cfg.enable {
    home.packages = [ cfg.package ];

    xdg.dataFile = mkMerge (
      mapAttrsToList (
        name: plugin:
        let
          info = getPluginInfo name plugin;
          builtPkg = buildPlugin name plugin;
        in
        {
          "lawnch/plugins/${name}.so" = {
            source = "${builtPkg}/lib/lawnch/plugins/${name}.so";
            executable = true;
          };
        }
        // (listToAttrs (
          map (asset: {
            name = "lawnch/assets/${asset}";
            value = {
              source = "${builtPkg}/lib/lawnch/assets/${asset}";
              recursive = true;
            };
          }) info.assets
        ))
      ) (filterAttrs (n: v: v.enable) cfg.plugins)
    );

    xdg.configFile."lawnch/config.ini".source = generateFullConfig;
  };
}
