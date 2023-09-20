#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>
#include <thread>

#define PRODUCER_KEY 0
#define CONSUMER_KEY 1
#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

void producerFc(HiCR::Backend* backend)
{
 // Asking backend to check the available resources
  backend->queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend->getMemorySpaceList();

  // Getting required buffer sizes
  auto coordinationBufferSize = HiCR::Channel::getCoordinationBufferSize();

  // Allocating memory slot for the coordination buffer
  auto coordinationBuffer = backend->allocateMemorySlot(*memSpaces.begin(), coordinationBufferSize);

  // Registering buffers globally for them to be used by remote actors
  auto globalBuffers = backend->exchangeGlobalMemorySlots(2, PRODUCER_KEY, {coordinationBuffer});

  // Creating producer and consumer channels
  auto producer = HiCR::ProducerChannel(backend, globalBuffers[CONSUMER_KEY][0], coordinationBuffer, sizeof(ELEMENT_TYPE), CAPACITY);

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer = 42;
  auto sendBufferPtr = &sendBuffer;
  auto sendSlot = backend->registerMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

  // Pushing value to the channel
  producer.push(sendSlot);

  // Printing values
  printf("Sent Value:     %u\n", *sendBufferPtr);

  // Freeing up memory
  backend->freeMemorySlot(coordinationBuffer);
  backend->deregisterMemorySlot(sendSlot);
}

void consumerFc(HiCR::Backend* backend)
{
 // Asking backend to check the available resources
  backend->queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend->getMemorySpaceList();

  // Getting required buffer sizes
  auto tokenBufferSize = HiCR::Channel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);

  // Allocating memory slot for the token buffer
  auto tokenBuffer = backend->allocateMemorySlot(*memSpaces.begin(), tokenBufferSize);

  // Registering buffers globally for them to be used by remote actors
  auto globalBuffers = backend->exchangeGlobalMemorySlots(2, CONSUMER_KEY, {tokenBuffer});

  // Creating producer and consumer channels
  auto consumer = HiCR::ConsumerChannel(backend, tokenBuffer, globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

  // Getting the value from the channel
  ELEMENT_TYPE* recvBufferPtr = NULL;
  bool peekResult = consumer.peek((void**)&recvBufferPtr);

  // If peek did not succeed, fail and exit
  if (peekResult == false) { fprintf(stderr, "Did not receive a message!\n"); exit(-1); }

  // Popping received value to free up channel
  consumer.pop();

  // Printing values
  printf("Received Value: %u\n", *recvBufferPtr);

  // Freeing up memory
  backend->freeMemorySlot(tokenBuffer);
}


int main(int argc, char **argv)
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend;

 auto producerThread = std::thread(producerFc, &backend);
 auto consumerThread = std::thread(consumerFc, &backend);

 producerThread.join();
 consumerThread.join();

 return 0;
}

