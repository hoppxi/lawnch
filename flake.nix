{
  description = "A lightweight launcher for Wayland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    lawnch-plugins.url = "github:hoppxi/lawnch-plugins";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      lawnch-plugins,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lawnch-plugin-api = lawnch-plugins.packages.${system}.plugin-api;

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
          lawnch-plugin-api
        ];

        desktopItem = pkgs.makeDesktopItem {
          name = "lawnch";
          desktopName = "Lawnch";
          exec = "lawnch";
          icon = "lawnch";
          comment = "A lightweight launcher for Wayland";
          type = "Application";
          categories = [ "Utility" ];
          terminal = false;
          noDisplay = true;
        };
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "lawnch";
          version = "0.1.0-alpha";

          src = ./.;

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
          ];

          nativeBuildInputs = nativeBuildPkgs ++ [ pkgs.copyDesktopItems ];
          buildInputs = buildPkgs;
          desktopItems = [ desktopItem ];

          enableParallelBuilding = true;
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = nativeBuildPkgs;
          buildInputs = buildPkgs;
        };
      }
    )
    // (
      let
        mkModule =
          platform:
          {
            config,
            lib,
            pkgs,
            ...
          }:
          let
            system = pkgs.stdenv.hostPlatform.system;
          in
          import ./nix/default.nix {
            inherit pkgs;
            lawnch = self.packages.${system}.default;
            available-plugins = lawnch-plugins.packages.${system};
            platform = platform;
          } { inherit config lib; };

      in
      {
        homeModules.default = mkModule "home-manager";
        nixosModules.default = mkModule "nixos";
      }
    );
}
