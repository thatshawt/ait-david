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
    sqlite
    mpfr
  ];

  shellHook = ''
    export LIBGMP_DIR=${pkgs.gmp}
    export LIBGMPXX_DIR=${pkgs.gmpxx}

    export SQLITE_INCLUDE=${pkgs.sqlite.dev}/include
    export SQLITE_LIB=${pkgs.sqlite.out}/lib

    export MPFR_INCLUDE=${pkgs.mpfr.dev}/include
    export MPFR_LIB=${pkgs.mpfr.out}/lib
    
    echo "ready for development >:)"
  '';
}
