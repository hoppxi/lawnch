{
  lib,
  stdenv,
  cmake,
}:

stdenv.mkDerivation {
  pname = "lawnch-plugin-api";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ cmake ];

  meta = with lib; {
    description = "API header for Lawnch plugins";
    license = licenses.mit;
  };
}
