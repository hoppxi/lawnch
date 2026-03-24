{
  description = "A lightweight launcher for Wayland";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    lawnch-plugins.url = "github:hoppxi/lawnch-plugins";
  };
  outputs = inputs: import ./nix inputs;
}
