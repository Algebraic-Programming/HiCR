#include <thread>
#include <hicr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>
#include <source/consumer.hpp>
#include <source/producer.hpp>

#define CONCURRENT_THREADS 2

int main(int argc, char **argv)
{
 // Instantiating backend
 HiCR::backend::sharedMemory::SharedMemory backend(CONCURRENT_THREADS);

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

 // Creating new threads (one for consumer, one for produer)
 auto consumerThread = std::thread(consumerFc, &backend, channelCapacity);
 auto producerThread = std::thread(producerFc, &backend, channelCapacity);

 // Waiting on threads
 consumerThread.join();
 producerThread.join();

 return 0;
}

