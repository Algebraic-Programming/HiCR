#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>
#include <backends/sharedMemory/pthreads/L1/computeManager.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>
#include <mpi.h>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing MPI
  MPI_Init(&argc, &argv);

  // Getting my rank
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Initializing host (CPU) topology manager
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  tm.queryDevices();

  // Getting first device (CPU) found
  auto d = *tm.getDevices().begin();

  // Obtaining memory spaces and compute resources
  auto memSpaces = d->getMemorySpaceList();
  auto computeResources = d->getComputeResourceList();

  // Creating MPI-based memory manager (necessary for passing data around between instances)
  HiCR::backend::mpi::L1::CommunicationManager communicationManager(MPI_COMM_WORLD);
  HiCR::backend::mpi::L1::MemoryManager memoryManager;

  // Initializing host (CPU) compute manager (for running incoming RPCs)
  HiCR::backend::sharedMemory::pthreads::L1::ComputeManager computeManager;

  // Getting first accesible memory space for buffering
  auto firstMemorySpace = *memSpaces.begin();

  // Getting first compute resource for running the RPCs
  auto firstComputeResource = *computeResources.begin();

  // Creating MPI-based instance manager
  HiCR::backend::mpi::L1::InstanceManager instanceManager(communicationManager, computeManager, memoryManager, firstMemorySpace);

  // Differentiating between coordinator and worker functions using the rank number
  if (rank == 0)
    coordinatorFc(instanceManager);
  else
    workerFc(instanceManager, computeManager, firstMemorySpace, firstComputeResource);

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
