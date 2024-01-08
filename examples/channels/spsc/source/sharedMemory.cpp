#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/pthreads/L1/communicationManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <thread>

#define CONCURRENT_THREADS 2

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 2)
  {
    fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
    return -1;
  }

  // Reading argument
  auto channelCapacity = std::atoi(argv[1]);

  // Capacity must be larger than zero
  if (channelCapacity == 0)
  {
    fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    return -1;
  }

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating HWLoc-based host (CPU) memory manager
  HiCR::backend::host::hwloc::L1::MemoryManager m(&topology);

  // Instantiating Pthread-based host (CPU) communication manager
  HiCR::backend::host::pthreads::L1::CommunicationManager c(CONCURRENT_THREADS);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  // Creating new threads (one for consumer, one for produer)
  auto consumerThread = std::thread([&]()
                                    { consumerFc(m, c, firstMemorySpace, channelCapacity); });
  auto producerThread = std::thread([&]()
                                    { producerFc(m, c, firstMemorySpace, channelCapacity); });

  // Waiting on threads
  consumerThread.join();
  producerThread.join();

  return 0;
}
