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

 auto consumerThread = std::thread(consumerFc, &backend);
 auto producerThread = std::thread(producerFc, &backend);

 consumerThread.join();
 producerThread.join();

 return 0;
}

