#include <thread>
#include <hicr.hpp>
#include <hicr/backends/sharedMemory/memoryManager.hpp>
#include <consumer.hpp>
#include <producer.hpp>

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
 HiCR::backend::sharedMemory::MemoryManager m(&topology, producerCount + 1);

 // Asking memory manager to check the available memory spaces
 m.queryMemorySpaces();

 // Creating single consumer thread
 auto consumerThread = std::thread(consumerFc, &m, channelCapacity, producerCount);

 // Creating producer threads
 std::vector<std::thread*> producerThreads(producerCount);
 for (size_t i = 0; i < producerCount; i++) producerThreads[i] = new std::thread(producerFc, &m, channelCapacity, i);

 // Waiting on threads
 consumerThread.join();
 for (size_t i = 0; i < producerCount; i++) producerThreads[i]->join();

 // Freeing memory
 for (size_t i = 0; i < producerCount; i++) delete producerThreads[i];

 return 0;
}

