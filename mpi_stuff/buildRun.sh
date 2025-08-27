rm -f cpi
mpic++ -o cpi cpi.c

export MPICH_PORT_RANGE="10000:10100"
mpirun -f hosts -np 10 ./cpi