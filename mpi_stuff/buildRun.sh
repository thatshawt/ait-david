rm -f cpi
mpic++ -o cpi cpi.c

export MPICH_PORT_RANGE="10000:10100"
mpirun -f machines -np 2 ./cpi