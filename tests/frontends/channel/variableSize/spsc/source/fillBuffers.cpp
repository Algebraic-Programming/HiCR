#include <fstream>
#include <mpi.h>
#include <nlohmann_json/json.hpp>

#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "channelFixture.hpp"
#include "producer.hpp"
#include "consumer.hpp"

TEST_F(ChannelFixture, fillBufferCounter)
{
  // Getting MPI values
  int rankCount = 0;
  int rankId    = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rankId);
  MPI_Comm_size(MPI_COMM_WORLD, &rankCount);

  // Sanity Check
  if (rankCount != 2)
  {
    if (rankId == 0) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
    MPI_Finalize();
  }

  // Reading argument
  size_t channelCapacity = CHANNEL_CAPACITY;

  // Instantiating backend
  HiCR::backend::mpi::MemoryManager        m;
  HiCR::backend::mpi::CommunicationManager c(MPI_COMM_WORLD);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  // Rank 0 is producer, Rank 1 is consumer
  if (rankId == 0) producerFc(m, m, c, c, firstMemorySpace, firstMemorySpace, channelCapacity);
  if (rankId == 1) consumerFc(m, m, c, c, firstMemorySpace, firstMemorySpace, channelCapacity);
}
