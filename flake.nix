{
  description = "ESP32-S3 Arduino (PlatformIO) dev shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # PlatformIO downloads prebuilt toolchains that expect a normal FHS
        # filesystem, so `pio` has to run inside one on NixOS.
        pio = pkgs.buildFHSEnv {
          name = "pio";
          targetPkgs = p: with p; [
            platformio-core
            python3
            git
            stdenv.cc.cc.lib
            zlib
            ncurses
            libusb1
            systemdMinimal
          ];
          runScript = "pio";
        };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = [ pio pkgs.git ];
        };
      });
}
