#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <backends/sharedMemory/hwloc/L1/memoryManager.hpp>
#include <backends/sharedMemory/pthreads/L1/communicationManager.hpp>
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>
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

  // Instantiating Shared Memory backend
  HiCR::backend::sharedMemory::hwloc::L1::MemoryManager m(&topology);

  // Instantiating shared memory backend's communication manager
  HiCR::backend::sharedMemory::pthreads::L1::CommunicationManager c(CONCURRENT_THREADS);

// Initializing Sequential backend's device manager
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Creating new threads (one for consumer, one for produer)
  auto consumerThread = std::thread(consumerFc, &m, &c, *memSpaces.begin(), channelCapacity);
  auto producerThread = std::thread(producerFc, &m, &c, *memSpaces.begin(), channelCapacity);

  // Waiting on threads
  consumerThread.join();
  producerThread.join();

  return 0;
}
