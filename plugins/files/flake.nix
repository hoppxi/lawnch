{
  description = "lawnch files plugin";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    lawnch-plugin-api.url = "path:../../packages/lawnch-plugin-api";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      lawnch-plugin-api,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        api = lawnch-plugin-api.packages.${system}.default;
      in
      {
        packages.default = pkgs.callPackage ./default.nix {
          lawnch-plugin-api = api;
        };
      }
    );
}
