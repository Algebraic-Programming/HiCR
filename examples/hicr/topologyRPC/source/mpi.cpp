#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <backends/mpi/L1/instanceManager.hpp>
#include <mpi.h>

int main(int argc, char **argv)
{
  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Creating MPI-based instance manager
  auto instanceManager = std::make_shared<HiCR::backend::mpi::L1::InstanceManager>(MPI_COMM_WORLD);

  // Get the locally running instance
  auto myInstance = instanceManager->getCurrentInstance();

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true) coordinatorFc(instanceManager);
  if (myInstance->isRootInstance() == false) workerFc(instanceManager);

  // Finalizing HiCR
  MPI_Finalize();

  return 0;
}
