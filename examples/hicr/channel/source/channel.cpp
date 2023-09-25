#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>
#include <thread>

#define CHANNEL_TAG 0
#define PRODUCER_KEY 0
#define CONSUMER_KEY 1
#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

void producerFc(HiCR::Backend* backend)
{
 // Obtaining memory spaces
 auto memSpaces = backend->getMemorySpaceList();

 // Getting required buffer sizes
 auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

 // Allocating memory slot for the coordination buffer (required at the producer side)
 auto coordinationBuffer = backend->allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::ProducerChannel::initializeCoordinationBuffer(backend, coordinationBuffer);

 // Registering buffers globally for them to be used by remote actors
 backend->exchangeGlobalMemorySlots(CHANNEL_TAG, PRODUCER_KEY, {coordinationBuffer});

 // Synchronizing so that all actors have finished registering their memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = backend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 42;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = backend->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing value to the channel
 producer.push(sendSlot);

 // Printing values
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Make sure the consumer got the message before exiting
 while(producer.getDepth() > 0) producer.checkReceiverPops();

 // Freeing up memory
 backend->deregisterLocalMemorySlot(sendSlot);
}

void consumerFc(HiCR::Backend* backend)
{
 // Obtaining memory spaces
 auto memSpaces = backend->getMemorySpaceList();

 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating memory slot for the token buffer (required at the consumer side)
 auto tokenBuffer = backend->allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Registering buffers globally for them to be used by remote actors
 backend->exchangeGlobalMemorySlots(CHANNEL_TAG, CONSUMER_KEY, {tokenBuffer});

 // Synchronizing so that all actors have finished registering their memory slots
 backend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = backend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(backend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Getting the value from the channel
 auto recvSlot = consumer.peekWait();

 // Calculating pointer to the value
 auto recvBuffer = (ELEMENT_TYPE*)backend->getLocalMemorySlotPointer(tokenBuffer);

 // Printing values
 printf("Received Value: %u\n", recvBuffer[recvSlot[0]]);

 // Popping received value to free up channel
 consumer.pop();

 // Freeing up memory
 backend->freeLocalMemorySlot(tokenBuffer);
}


int main(int argc, char **argv)
{
 // Instantiating backend
 HiCR::backend::sequential::Sequential backend(2);

 // Asking backend to check the available resources
 backend.queryMemorySpaces();

 auto consumerThread = std::thread(consumerFc, &backend);
 auto producerThread = std::thread(producerFc, &backend);

 consumerThread.join();
 producerThread.join();

 return 0;
}

