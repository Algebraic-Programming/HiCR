#include <mpi.h>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "include/consumer.hpp"
#include "include/producer.hpp"

int main(int argc, char **argv)
{
  // Initializing MPI
  MPI_Init(&argc, &argv);

  // Getting MPI values
  int rankCount = 0;
  int rankId    = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rankId);
  MPI_Comm_size(MPI_COMM_WORLD, &rankCount);

  // Sanity Check
  if (rankCount < 2)
  {
    if (rankId == 0) fprintf(stderr, "Launch error: MPI process count must be at least 2\n");
    return MPI_Finalize();
  }

  // Checking arguments
  if (argc != 2)
  {
    if (rankId == 0) fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
    return MPI_Finalize();
  }

  // Reading argument
  auto channelCapacity = std::atoi(argv[1]);

  // Capacity must be larger than zero
  if (channelCapacity == 0)
  {
    if (rankId == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    return MPI_Finalize();
  }

  // Instantiating backend
  HiCR::backend::mpi::L1::MemoryManager        m;
  HiCR::backend::mpi::L1::CommunicationManager c(MPI_COMM_WORLD);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting reference to the first memory space detected
  auto firstMemorySpace = *memSpaces.begin();

  // Calculating the number of producer processes
  size_t producerCount = rankCount - 1;

  // Rank 0 is consumer, the rest are producers
  if (rankId == 0) consumerFc(m, c, firstMemorySpace, channelCapacity, producerCount);
  if (rankId >= 1) producerFc(m, c, firstMemorySpace, channelCapacity, rankId - 1, producerCount);

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
