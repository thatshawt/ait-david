{
  pkgs ? import <nixpkgs> {},
}:
let
  acl2-no-books = pkgs.callPackage pkgs.acl2 {
    certifyBooks = false;
  };
in
pkgs.mkShell {
  name = "acl2-fun-shell";

  # Define the packages to be available in the environment
  packages = with pkgs; [
    acl2-no-books
    gnumake
  ];

  shellHook = ''
    echo "acl2 ready for duty."
  '';
}
