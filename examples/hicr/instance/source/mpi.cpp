#include <mpi.h>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <include/coordinator.hpp>
#include <include/worker.hpp>

int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Creating MPI-based instance manager
 HiCR::backend::mpi::InstanceManager instanceManager(MPI_COMM_WORLD);

 // Differentiating between coordinator and worker functions
 if (instanceManager.isCoordinatorInstance())
  coordinatorFc(instanceManager);
 else
  workerFc(instanceManager);

 // Finalizing MPI
 MPI_Finalize();

 return 0;
}

