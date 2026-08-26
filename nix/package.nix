{ pkgs, lawnch-plugin-api, ... }:

let
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
pkgs.stdenv.mkDerivation {
  pname = "lawnch";
  version = "0.5.0-alpha";

  src = ../.;

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
  ];

  nativeBuildInputs = nativeBuildPkgs ++ [ pkgs.copyDesktopItems ];
  buildInputs = buildPkgs;
  desktopItems = [ desktopItem ];

  enableParallelBuilding = true;

  meta = with pkgs.lib; {
    description = "A lightweight launcher for Wayland";
    homepage = "https://github.com/hoppxi/lawnch";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}
