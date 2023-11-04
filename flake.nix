{
  description = "HPC PS Dev Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        darwinInputs = with pkgs;
          lib.optionals stdenv.isDarwin
            (with pkgs.darwin.apple_sdk.frameworks; [ ]);
        pkgs = import nixpkgs { inherit system; };
      in
      with pkgs; {
        devShells.default = mkShell { buildInputs = [ mpi ] ++ darwinInputs; };
      });
}
