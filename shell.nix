let
  pkgs = import <nixpkgs> { };
  lsp_regen = pkgs.writeShellScriptBin "lsp_regen"
    "pio run -t compiledb -e lolin32; ./generate_dotclangd.py";
in pkgs.mkShell {
  name = "Teslasynth";
  buildInputs = with pkgs; [
    lsp_regen
    platformio
    ruff
    tio
    rosegarden
    alsa-utils
  ];
}
