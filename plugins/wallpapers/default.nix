{
  pkgs,
  lawnch-plugin-api ? null,
  ...
}:

let
  api = lawnch-plugin-api;
  pluginName = "wallpapers";
in
pkgs.stdenv.mkDerivation {
  pname = "lawnch-plugin-${pluginName}";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ pkgs.cmake ];
  buildInputs = [ api ];

  cmakeFlags = [
    "-DLawnchPluginApi_DIR=${api}/lib/cmake/LawnchPluginApi"
  ];

  installPhase = ''
    mkdir -p $out/lib/lawnch/plugins
    cp *.so $out/lib/lawnch/plugins/

    mkdir -p $out/lib/lawnch/plugins/pinfo
    cp $src/pinfo $out/lib/lawnch/plugins/pinfo/${pluginName}
  '';
}
