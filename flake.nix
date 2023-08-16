{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
      clang-tool = {
        name,
        flags ? "",
        extra-args ? "",
      }:
        pkgs.writeShellScriptBin name ''
          ${pkgs.clang-tools_16}/bin/${name} ${flags} "$@" ${extra-args}
        '';
    in {
      devShells.default = pkgs.mkShell {
        name = "graphs";

        packages = with pkgs; [
          gnumake
          (clang-tool {
            name = "clang-tidy";
            extra-args =
              if stdenv.isDarwin
              then "-- -I${pkgs.darwin.Libsystem}"
              else "";
          })
          (clang-tool {name = "clang-format";})
          clang_16
        ];
      };
    });
}
