 #include <mpi.h>
#include "include/common.hpp"
#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>

int main(int argc, char *argv[])
{
  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Creating MPI-based instance manager
  HiCR::backend::mpi::L1::InstanceManager instanceManager(MPI_COMM_WORLD);

  // Get the locally running instance
  auto myInstance = instanceManager.getCurrentInstance();

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    if (myInstance->isRootInstance()) fprintf(stderr, "Launch error. No machine model file provided\n");
    instanceManager.finalize();
    return -1;
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true) coordinatorFc(instanceManager, machineModelFile);
  if (myInstance->isRootInstance() == false) workerFc(instanceManager);

  // Finalizing HiCR
  instanceManager.finalize();

  return 0;
}
