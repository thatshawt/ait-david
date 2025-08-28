rm -f cpi hello_world
mpic++ -o cpi cpi.c
mpic++ -o hello_world hello_world.c

export MPICH_PORT_RANGE="10000:10100"
# export MPICH_DBG_OUTPUT="VERBOSE"
# export MPICH_DBG_CLASS="ALL"
# export I_MPI_DEBUG=10
# export MPIR_CVAR_DEBUG_SUMMARY=1
# export FI_LOG_LEVEL=Info
export FI_PROVIDER=tcp
export FI_TCP_IFACE=dummy0

# add -verbose if you want that for debugging
mpiexec -launcher ssh -f machines -np 50 ./cpi