#include <thread>
#include <hicr.hpp>
#include <hicr/backends/sequential/sequential.hpp>
#include <source/consumer.hpp>
#include <source/producer.hpp>

int main(int argc, char **argv)
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend(2);

 auto consumerThread = std::thread(consumerFc, &backend);
 auto producerThread = std::thread(producerFc, &backend);

 consumerThread.join();
 producerThread.join();

 return 0;
}

