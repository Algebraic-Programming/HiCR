#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <backends/sequential/L1/memoryManager.hpp>
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

  // Instantiating backend
  HiCR::backend::sequential::L1::MemoryManager m(CONCURRENT_THREADS);

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
