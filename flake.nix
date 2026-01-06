{
  description = "A lightweight launcher for Wayland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    let
      inherit (nixpkgs) lib;

      pluginsDir = ./plugins;
      knownPlugins =
        if builtins.pathExists pluginsDir then
          lib.mapAttrs (name: type: "${pluginsDir}/${name}") (
            lib.filterAttrs (n: v: v == "directory") (builtins.readDir pluginsDir)
          )
        else
          { };
    in
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        nativeBuildPkgs = with pkgs; [
          gcc
          cmake
          pkg-config
          wayland-scanner
          wayland-protocols
          wlr-protocols
        ];

        buildPkgs = with pkgs; [
          wayland
          cairo
          pango
          nlohmann_json
          curl
          inih
          harfbuzz
          libxdmcp
          libxkbcommon
          librsvg
          fontconfig
          expat
          libglvnd
        ];

        lawnch-unwrapped = pkgs.stdenv.mkDerivation {
          pname = "lawnch-unwrapped";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = nativeBuildPkgs;
          buildInputs = buildPkgs;

          installPhase = ''
            runHook preInstall
            mkdir -p $out/bin
            cp lawnch $out/bin/lawnch

            mkdir -p $out/include/lawnch/plugins
            cp $src/plugins/lawnch_plugin_api.h $out/include/lawnch/plugins/

            runHook postInstall
          '';
        };

        lawnch = pkgs.writeShellScriptBin "lawnch" ''
          ${lawnch-unwrapped}/bin/lawnch "$@"
        '';
      in
      {
        packages.default = lawnch;
        packages.lawnch-unwrapped = lawnch-unwrapped;

        lib.knownPlugins = knownPlugins;

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = nativeBuildPkgs;
          buildInputs = buildPkgs;

          shellHook = ''
            export WLR_PROTOCOLS_DIR=${pkgs.wlr-protocols}
          '';
        };
      }
    )
    // {
      homeModules.default =
        {
          config,
          lib,
          pkgs,
          ...
        }:
        let
          system = pkgs.stdenv.hostPlatform.system;
          lawnchPackage = self.packages.${system}.default;
          lawnchUnwrappedPackage = self.packages.${system}.lawnch-unwrapped;
        in
        import ./nix/default.nix {
          inherit pkgs knownPlugins;
          lawnch = lawnchPackage;
          lawnch-unwrapped = lawnchUnwrappedPackage;
        } { inherit config lib; };
    };
}
