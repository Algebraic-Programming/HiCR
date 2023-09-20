#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>
#include <thread>

#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

void producerFc(HiCR::Backend* backend)
{
 // Asking backend to check the available resources
  backend->queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend->getMemorySpaceList();

  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::Channel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);
  auto coordinationBufferSize = HiCR::Channel::getCoordinationBufferSize();

  // Allocating memory slot for the channel buffer
  auto tokenBuffer        = backend->allocateMemorySlot(*memSpaces.begin(), tokenBufferSize);
  auto coordinationBuffer = backend->allocateMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Creating producer and consumer channels
  auto producer = HiCR::ProducerChannel(backend, tokenBuffer, coordinationBuffer, sizeof(ELEMENT_TYPE), CAPACITY);
  auto consumer = HiCR::ConsumerChannel(backend, tokenBuffer, coordinationBuffer, sizeof(ELEMENT_TYPE), CAPACITY);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer = 42;
  auto sendBufferPtr = &sendBuffer;
  auto sendSlot = backend->registerMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing value to the channel
  producer.push(sendSlot);

  // Getting the value from the channel
  ELEMENT_TYPE* recvBufferPtr = NULL;
  bool peekResult = consumer.peek((void**)&recvBufferPtr);

  // If peek did not succeed, fail and exit
  if (peekResult == false) { fprintf(stderr, "Did not receive a message!\n"); exit(-1); }

  // Popping received value to free up channel
  consumer.pop();

  // Printing values
  printf("Sent Value:     %u\n", *sendBufferPtr);
  printf("Received Value: %u\n", *recvBufferPtr);

  // Freeing up memory
  backend->freeMemorySlot(tokenBuffer);
  backend->freeMemorySlot(coordinationBuffer);
  backend->deregisterMemorySlot(sendSlot);
}


int main(int argc, char **argv)
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend;

 auto producerThread = std::thread(producerFc, &backend);

 producerThread.join();
 return 0;
}

