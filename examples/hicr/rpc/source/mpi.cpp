#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <backends/sequential/L1/deviceManager.hpp>
#include <backends/sequential/L1/computeManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>
#include <mpi.h>

int main(int argc, char **argv)
{
  // Initializing MPI
  MPI_Init(&argc, &argv);

  // Getting my rank
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces and compute resources
  auto memSpaces = d->getMemorySpaceList();
  auto computeResources = d->getComputeResourceList();

  // Creating MPI-based memory manager (necessary for passing data around between instances)
  HiCR::backend::mpi::L1::CommunicationManager communicationManager(MPI_COMM_WORLD);
  HiCR::backend::mpi::L1::MemoryManager memoryManager;
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Creating MPI-based instance manager
  HiCR::backend::mpi::L1::InstanceManager instanceManager(&communicationManager, &computeManager,  &memoryManager, *memSpaces.begin());

  // Differentiating between coordinator and worker functions using the rank number
  if (rank == 0)
    coordinatorFc(instanceManager);
  else
    workerFc(instanceManager, computeManager, *memSpaces.begin(), *computeResources.begin());

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
