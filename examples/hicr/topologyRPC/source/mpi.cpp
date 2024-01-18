#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>
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
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Getting my rank
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Initializing host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager topologyManager(&topology);

  // Asking backend to check the available devices
  const auto t = topologyManager.queryTopology();

  // Getting first device (CPU) found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces and compute resources
  auto memSpaces = d->getMemorySpaceList();
  auto computeResources = d->getComputeResourceList();

  // Getting first accesible memory space for buffering
  auto firstMemorySpace = *memSpaces.begin();

  // Getting first compute resource for running the RPCs
  auto firstComputeResource = *computeResources.begin();

  // Creating MPI-based communication manager (necessary for passing data around between instances)
  auto communicationManager = std::make_shared<HiCR::backend::mpi::L1::CommunicationManager>(MPI_COMM_WORLD);

  // Creating MPI-based memory manager (necessary for buffer allocation)
  auto memoryManager = std::make_shared<HiCR::backend::mpi::L1::MemoryManager>();

  // Initializing host (CPU) compute manager (for running incoming RPCs)
  auto computeManager = std::make_shared<HiCR::backend::host::pthreads::L1::ComputeManager>();

  // Creating MPI-based instance manager
  auto instanceManager = std::make_shared<HiCR::backend::mpi::L1::InstanceManager>(communicationManager, computeManager, memoryManager);

  // Setting buffer memory space for message exchanges
  instanceManager->setBufferMemorySpace(firstMemorySpace);

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager->createProcessingUnit(firstComputeResource);

  // Assigning processing unit to the instance manager
  instanceManager->addProcessingUnit(std::move(processingUnit));

  // Differentiating between coordinator and worker functions using the rank number
  if (rank == 0)
    coordinatorFc(instanceManager);
  else
    workerFc(instanceManager, computeManager);

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
