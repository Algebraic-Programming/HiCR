#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>
#include <thread>

#define CHANNEL_TAG 0
#define PRODUCER_KEY 0
#define CONSUMER_KEY 1
#define EXPECTED_GLOBAL_BUFFER_COUNT 2
#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

void producerFc()
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend;

 // Asking backend to check the available resources
 backend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = backend.getMemorySpaceList();

 // Getting required buffer sizes
 auto coordinationBufferSize = HiCR::Channel::getCoordinationBufferSize();

 // Allocating memory slot for the coordination buffer
 auto coordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Registering buffers globally for them to be used by remote actors
 auto globalBuffers = backend.exchangeGlobalMemorySlots(CHANNEL_TAG, EXPECTED_GLOBAL_BUFFER_COUNT, PRODUCER_KEY, {coordinationBuffer});

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(&backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 42;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = backend.registerMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing value to the channel
 producer.push(sendSlot);

 // Printing values
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Make sure the consumer got the message before exiting
 while(producer.getDepth() > 0) producer.checkReceiverPops();

 // Freeing up memory
 backend.freeMemorySlot(coordinationBuffer);
 backend.deregisterMemorySlot(sendSlot);
 backend.deregisterMemorySlot(globalBuffers[CONSUMER_KEY][0]);
 backend.deregisterMemorySlot(globalBuffers[PRODUCER_KEY][0]);
}

void consumerFc()
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend;

 // Asking backend to check the available resources
 backend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = backend.getMemorySpaceList();

 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::Channel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating memory slot for the token buffer
 auto tokenBuffer = backend.allocateMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Registering buffers globally for them to be used by remote actors
 auto globalBuffers = backend.exchangeGlobalMemorySlots(CHANNEL_TAG, EXPECTED_GLOBAL_BUFFER_COUNT, CONSUMER_KEY, {tokenBuffer});

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(&backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Getting the value from the channel
 ELEMENT_TYPE* recvBufferPtr = NULL;
 auto recvSlot = backend.registerMemorySlot(&recvBufferPtr, sizeof(ELEMENT_TYPE*));
 consumer.peekWait(recvSlot);

 // Popping received value to free up channel
 consumer.pop();

 // Printing values
 printf("Received Value: %u\n", *recvBufferPtr);

 // Freeing up memory
 backend.freeMemorySlot(tokenBuffer);
 backend.deregisterMemorySlot(recvSlot);
 backend.deregisterMemorySlot(globalBuffers[CONSUMER_KEY][0]);
 backend.deregisterMemorySlot(globalBuffers[PRODUCER_KEY][0]);
}


int main(int argc, char **argv)
{
 auto consumerThread = std::thread(consumerFc);
 auto producerThread = std::thread(producerFc);

 consumerThread.join();
 producerThread.join();

 return 0;
}

