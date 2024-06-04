{
  description = "WebNowPlaying-CLI";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
  outputs = inputs@{ self, flake-parts, ... }: 
  flake-parts.lib.mkFlake { inherit inputs; } {
    systems = [
      "x86_64-linux"
      "aarch64-linux"
    ];
    perSystem = { pkgs, system, ...}: {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "mpscd";
        version = builtins.readFile ./VERSION;
        src = ./.;
        buildInputs = with pkgs; [ clang makeWrapper ];
        buildPhase = ''
          make
        '';
        installPhase = ''
          mkdir -p $out/bin
          cp build/mpscd $out/bin/mpscd
        '';
      };
      devShells.default = pkgs.mkShell {
        shellHook = "exec $SHELL";
        buildInputs = with pkgs; [ clang valgrind gnumake ];
      };
    };
    flake = {
      homeManagerModules.mpscd = { config, pkgs, lib, ... }: let
        inherit (pkgs.stdenv.hostPlatform) system;
      in {
        options.programs.mpscd = {
          enable = lib.mkOption {
            type = lib.types.bool;
            default = false;
            description = "Install mpscd package";
          };
          package = lib.mkOption {
            type = lib.types.package;
            default = self.packages.${system}.default;
            description = "mpscd package to install";
          };
        };
        config = lib.mkIf config.programs.mpscd.enable {
          home.packages = [ config.programs.mpscd.package ];
        };
      };
    };
  };
}
