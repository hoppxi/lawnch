{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    cmake
    pkg-config
    gtk4
    gtk4-layer-shell
    nlohmann_json

    curl
    bc
    wl-clipboard
    cliphist
    findutils
  ];

  shellHook = ''
    export CXXFLAGS="-I${pkgs.curl.dev}/include $CXXFLAGS"
    export LDFLAGS="-L${pkgs.curl.dev}/lib $LDFLAGS"

    echo "Environment ready. Build with:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  make"
    echo "  ./lawnch"
  '';
}
