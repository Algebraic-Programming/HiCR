#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include <mpi.h>

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
  if (rankCount != 2)
  {
    if (rankId == 0) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
    return MPI_Finalize();
  }

  // Checking arguments
  if (argc != 4)
  {
    if (rankId == 0)
    {
      fprintf(stderr, "Error: Must provide <channel capacity> <message count> <token size in bytes> as arguments.\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
  }

  // Reading argument
  auto channelCapacity = std::atoi(argv[1]);
  auto msgCount        = std::atoi(argv[2]);
  auto tokenSize       = std::atoi(argv[3]);

  // Capacity must be larger than zero
  if (channelCapacity == 0)
  {
    if (rankId == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    return MPI_Finalize();
  }

  // Instantiating backend
  HiCR::backend::mpi::MemoryManager        m;
  HiCR::backend::mpi::CommunicationManager c(MPI_COMM_WORLD);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  auto start = MPI_Wtime();

  // Rank 0 is producer, Rank 1 is consumer
  if (rankId == 0) producerFc(m, c, firstMemorySpace, channelCapacity, msgCount, tokenSize);
  if (rankId == 1) consumerFc(m, c, firstMemorySpace, channelCapacity, msgCount, tokenSize);

  auto end = MPI_Wtime();
  if (rankId == 0) printf("Time: %lf seconds\n", end - start);

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}
