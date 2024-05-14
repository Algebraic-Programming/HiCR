#include <consumer.hpp>
#include <hicr.hpp>
#include <hicr/backends/sequential/memoryManager.hpp>
#include <producer.hpp>
#include <thread>

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 3)
  {
    fprintf(stderr, "Error: Must provide the channel capacity and number of producers as arguments.\n");
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

  auto numProducers = std::atoi(argv[2]);
  if (numProducers < 1)
  {
    fprintf(stderr, "Error: Number of producers should be greater than zero.\n");
    return -1;
  }

  // Total threads are 1 consumer plus number of producers specified
  auto totalThreads = numProducers + 1;

  // Instantiating backend
  HiCR::backend::sequential::MemoryManager m(totalThreads);

  // Asking memory manager to check the available memory spaces
  m.queryMemorySpaces();

  // Creating new threads (one for consumer, the rest for producers)
  auto consumerThread = std::thread(consumerFc, &m, channelCapacity, numProducers);

  std::vector<std::thread *> producerThreads(numProducers);
  for (auto i = 0; i < numProducers; i++) producerThreads[i] = new std::thread([&]() { producerFc(m, c, firstMemorySpace, channelCapacity, i); });

  // Waiting on threads
  consumerThread.join();
  for (auto i = 0; i < numProducers; i++) producerThreads[i]->join();

  // Freeing memory
  for (size_t i = 0; i < producerCount; i++) delete producerThreads[i];

  return 0;
}
