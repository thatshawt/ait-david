{
  pkgs ? import <nixpkgs> {}
}:
pkgs.mkShell {
  name = "chicken-spike-shell";

  # Define the packages to be available in the environment
  packages = with pkgs; [
    gcc
    chicken
    gnumake
  ];

  shellHook = ''
    echo "Chicken ( •ө• ) Spike Ready For Duty."
  '';
}
