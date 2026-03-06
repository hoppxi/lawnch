{
  description = "A lightweight launcher for Wayland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    lawnch-plugins.url = "github:/hoppxi/lawnch-plugins";
  };

  outputs =
    {
      self,
      nixpkgs,
      lawnch-plugins,
      ...
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = nixpkgs.lib.genAttrs systems;

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
          inherit pkgs platform;
          lawnch = self.packages.${system}.default;
          available-plugins = lawnch-plugins.packages.${system};
        } { inherit config lib; };
    in
    {
      packages = forAllSystems (
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
            tomlplusplus
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
          default = pkgs.stdenv.mkDerivation {
            pname = "lawnch";
            version = "0.2.0-alpha";

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
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
          };
        }
      );

      homeModules.default = mkModule "home-manager";
      nixosModules.default = mkModule "nixos";
    };
}
