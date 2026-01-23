{
  description = "A lightweight launcher for Wayland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    lawnch-plugin-api.url = "path:./packages/lawnch-plugin-api";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      lawnch-plugin-api,
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

        lawnch-plugin-api-pkg = lawnch-plugin-api.packages.${system}.default;

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
          blend2d
          inih
          libxkbcommon
          nanosvg
          fontconfig
          libffi
          expat
          lawnch-plugin-api-pkg
        ];

        lawnch-unwrapped = pkgs.stdenv.mkDerivation {
          pname = "lawnch-unwrapped";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = nativeBuildPkgs;
          buildInputs = buildPkgs;

          enableParallelBuilding = true;

          installPhase = ''
            runHook preInstall
            mkdir -p $out/bin
            cp lawnch $out/bin/lawnch

            mkdir -p $out/lib/lawnch/
            cp -r $src/themes $out/lib/lawnch/

            runHook postInstall
          '';
        };

        lawnch = pkgs.writeShellScriptBin "lawnch" ''
          ${lawnch-unwrapped}/bin/lawnch "$@"
        '';
      in
      {
        packages.default = lawnch;
        packages.lawnch-plugin-api = lawnch-plugin-api-pkg;
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
          lawnch-plugin-api = self.packages.${system}.lawnch-plugin-api;
        } { inherit config lib; };
    };
}
