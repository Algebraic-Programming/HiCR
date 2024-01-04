#include <backends/sharedMemory/hwloc/L1/memoryManager.hpp>
#include <backends/sharedMemory/pthreads/L1/communicationManager.hpp>
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>
#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <hwloc.h>
#include <thread>

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 3)
  {
    fprintf(stderr, "Error: Must provide the channel capacity and producer count as arguments.\n");
    fprintf(stderr, "Example: ./sharedMemory 3 4 # Creates a channel of capacity 3, and 4 producers.\n");
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

  // Getting number of threads to create from argument
  size_t producerCount = std::atoi(argv[2]);

  // Thread count must be at least 2
  if (producerCount < 1)
  {
    fprintf(stderr, "Error: The number of producer threads must be at least 1.\n");
    return -1;
  }

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating Shared Memory backend
  HiCR::backend::sharedMemory::hwloc::L1::MemoryManager m(&topology);

    // Instantiating Shared Memory communication backend
  HiCR::backend::sharedMemory::pthreads::L1::CommunicationManager c(producerCount + 1);

// Initializing Sequential backend's topology manager
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Creating single consumer thread
  auto consumerThread = std::thread(consumerFc, &m, &c, *memSpaces.begin(), channelCapacity, producerCount);

  // Creating producer threads
  std::vector<std::thread *> producerThreads(producerCount);
  for (size_t i = 0; i < producerCount; i++) producerThreads[i] = new std::thread(producerFc, &m, &c, *memSpaces.begin(), channelCapacity, i);

  // Waiting on threads
  consumerThread.join();
  for (size_t i = 0; i < producerCount; i++) producerThreads[i]->join();

  // Freeing memory
  for (size_t i = 0; i < producerCount; i++) delete producerThreads[i];

  return 0;
}
