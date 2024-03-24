{
  description = "A flake for ssss (Shamir's Secret Sharing Scheme)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; };
      in {
        packages.ssss = pkgs.callPackage ./default.nix { };
        defaultPackage = self.packages.${system}.ssss;
        apps.ssss-combine = {
          type = "app";
          program = "${self.packages.${system}.ssss}/bin/ssss-combine";
        };
        apps.ssss-split = {
          type = "app";
          program = "${self.packages.${system}.ssss}/bin/ssss-split";
        };
      });
}

