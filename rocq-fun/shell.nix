{
  pkgs ? import <nixpkgs> {},
  # unstablePkgs ? import <nixos-unstable> {},
  unstablePkgs ? import (builtins.fetchGit {
    # Descriptive name to make the store path easier to identify
    name = "coqPackages: add coqWithPackages/coqWithPackages' functions";
    url = "https://github.com/nixos/nixpkgs/";
    ref = "refs/heads/nixos-unstable";
    rev = "a776193f6bc7a9bc8c21ebb35e2e52e542cf3ea8";
  }) {},
}:
pkgs.mkShell {
  name = "rocq-fun-shell";

  # Define the packages to be available in the environment
  packages = with unstablePkgs; [
    gnumake
    # coq
    # coqPackages.stdlib
    # coqPackages.mathcomp
    # coqPackages.bignums
    coq.withPackages (
      ps: with ps; [
        stdlib
        # mathcomp
        # bignums
      ]
    )
  ];

  shellHook = ''
    echo "rocq shell ready for duty."
  '';
}
