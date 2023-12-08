#include <thread>
#include <hicr/backends/sharedMemory/L1/memoryManager.hpp>
#include "include/consumer.hpp"
#include "include/producer.hpp"

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
 HiCR::backend::sharedMemory::L1::MemoryManager m(&topology, CONCURRENT_THREADS);

 // Asking memory manager to check the available memory spaces
 m.queryMemorySpaces();

 // Creating new threads (one for consumer, one for produer)
 auto consumerThread = std::thread(consumerFc, &m, channelCapacity);
 auto producerThread = std::thread(producerFc, &m, channelCapacity);

  // Waiting on threads
  consumerThread.join();
  producerThread.join();

  return 0;
}
