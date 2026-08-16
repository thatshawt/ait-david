{ pkgs ? import <nixos-unstable> {} }:
pkgs.mkShell {
  # Set the name of the shell
  name = "my-dev-shell";

  # Define the packages to be available in the environment
  packages = [
    pkgs.zig_0_16
  ];

  shellHook = ''
    echo "turing3 locked and loaded MEOWWWWW ^ↀᴥↀ^"
  '';
}
