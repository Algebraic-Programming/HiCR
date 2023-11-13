#include <mpi.h>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <include/coordinator.hpp>
#include <include/worker.hpp>

int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Getting my rank
 int rank;
 MPI_Comm_rank(MPI_COMM_WORLD, &rank);

 // Creating MPI-based memory manager (necessary for passing data around between instances)
 HiCR::backend::mpi::MemoryManager memoryManager(MPI_COMM_WORLD);

 // Creating MPI-based instance manager
 HiCR::backend::mpi::InstanceManager instanceManager(&memoryManager);

 // Differentiating between coordinator and worker functions using the rank number
 if (rank == 0)
  coordinatorFc(instanceManager);
 else
  workerFc(instanceManager);

 // Finalizing MPI
 MPI_Finalize();

 return 0;
}

