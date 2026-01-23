{
  pkgs,
  lawnch-plugin-api,
  ...
}:

let
  api = lawnch-plugin-api;
  pluginName = "emoji";
in
pkgs.stdenv.mkDerivation {
  pname = "lawnch-plugin-${pluginName}";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ pkgs.cmake ];
  buildInputs = [
    api
    pkgs.nlohmann_json
  ];

  cmakeFlags = [
    "-DLawnchPluginApi_DIR=${api}/lib/cmake/LawnchPluginApi"
  ];

  installPhase = ''
    mkdir -p $out/lib/lawnch/plugins
    cp *.so $out/lib/lawnch/plugins/

    mkdir -p $out/lib/lawnch/plugins/pinfo
    cp $src/pinfo $out/lib/lawnch/plugins/pinfo/${pluginName}

    mkdir -p $out/lib/lawnch/plugins/assets
    cp $src/emoji.json $out/lib/lawnch/plugins/assets/emoji.json
  '';
}
