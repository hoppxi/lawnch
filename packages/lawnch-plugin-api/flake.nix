{
  description = "Lawnch Plugin API";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "lawnch-plugin-api";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = [ pkgs.cmake ];

          meta = with pkgs.lib; {
            description = "API header for Lawnch plugins";
            license = licenses.mit;
          };
        };
      }
    );
}
