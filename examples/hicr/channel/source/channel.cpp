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
 auto sendSlot = backend.allocateMemorySlot(*memSpaces.begin(), sizeof(ELEMENT_TYPE));
 auto sendBuffer = (ELEMENT_TYPE*)backend.getMemorySlotLocalPointer(sendSlot);
 *sendBuffer = 42;

 // Pushing value to the channel
 producer.push(sendSlot);

 // Getting the value from the channel
// double recvValue = 0.0;
 //consumer.peek(&recvValue);
 //consumer.pop();

 // Printing values
 printf("Sent Value:     %u\n", *sendBuffer);
 //printf("Received Value: %u\n", recvValue);

 return 0;
}

