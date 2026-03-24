{ pkgs, lawnch }:

pkgs.mkShell {
  inputsFrom = [ lawnch ];
}
