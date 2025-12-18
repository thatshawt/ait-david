{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  # Set the name of the shell
  name = "my-dev-shell";

  # Define the packages to be available in the environment
  packages = with pkgs; [
    gcc
    gdb
    valgrind
    gnumake
    gmp
  ];

  # Optional: commands to run when entering the shell
  shellHook = ''
    echo "ready for development >:)"
    export LIBGMP_DIR=${pkgs.gmp}
    export LIBGMPXX_DIR=${pkgs.gmpxx}
  '';
}
