#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>

#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

int main(int argc, char **argv)
{
 // Instantiating Shared Memory backend
 HiCR::backend::sequential::Sequential backend;

 // Asking backend to check the available resources
 backend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = backend.getMemorySpaceList();

 // Allocating memory slot for the channel buffer
 auto dataBuffer         = backend.allocateMemorySlot(*memSpaces.begin(), CAPACITY * sizeof(ELEMENT_TYPE));
 auto coordinationBuffer = backend.allocateMemorySlot(*memSpaces.begin(), HiCR::Channel::getCoordinationBufferSize());

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(&backend, dataBuffer, coordinationBuffer, sizeof(ELEMENT_TYPE));
 auto consumer = HiCR::ConsumerChannel(&backend, dataBuffer, coordinationBuffer, sizeof(ELEMENT_TYPE));

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 42;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = backend.registerMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing value to the channel
 producer.push(sendSlot);

 // Checking for new tokens on the receiver side
 consumer.checkReceivedTokens();

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
 backend.freeMemorySlot(dataBuffer);
 backend.freeMemorySlot(coordinationBuffer);
 backend.deregisterMemorySlot(sendSlot);

 return 0;
}

