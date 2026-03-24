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
    import ./module.nix {
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
    in
    {
      default = import ./package.nix { inherit pkgs lawnch-plugin-api; };
    }
  );

  devShells = forAllSystems (
    system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
      lawnch = self.packages.${system}.default;
    in
    {
      default = import ./devshell.nix { inherit pkgs lawnch; };
    }
  );

  homeModules = {
    default = mkModule "home-manager";
    lawnch = mkModule "home-manager";
  };

  nixosModules = {
    default = mkModule "nixos";
    lawnch = mkModule "nixos";
  };
}
