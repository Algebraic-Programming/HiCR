#include <hicr/backends/mpi/mpi.hpp>
#include <hicr/backends/sequential/sequential.hpp>
#include <hicr.hpp>
#include <mpi.h>

#define CHANNEL_TAG 0
#define PRODUCER_KEY 0
#define CONSUMER_KEY 1
#define CAPACITY 256
#define ELEMENT_TYPE unsigned int

void producerFc(HiCR::Backend* MPIbackend)
{
 // Instantiating backend for memory allocation
 HiCR::backend::sequential::Sequential sequentialBackend;

 // Asking backend to check the available resources
 sequentialBackend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = sequentialBackend.getMemorySpaceList();

 // Getting required buffer sizes
 auto coordinationBufferSize = HiCR::ProducerChannel::getCoordinationBufferSize();

 // Allocating memory slot for the coordination buffer (required at the producer side)
 auto coordinationBufferAlloc = sequentialBackend.allocateLocalMemorySlot(*memSpaces.begin(), coordinationBufferSize);

 // Getting local pointer of coordination buffer
 auto coordinationBufferPtr = sequentialBackend.getLocalMemorySlotPointer(coordinationBufferAlloc);

 // Registering coordination buffer as MPI memory slot
 auto coordinationBuffer = MPIbackend->registerLocalMemorySlot(coordinationBufferPtr, coordinationBufferSize);

 // Initializing coordination buffer (sets to zero the counters)
 HiCR::ProducerChannel::initializeCoordinationBuffer(MPIbackend, coordinationBuffer);

 // Registering buffers globally for them to be used by remote actors
 MPIbackend->exchangeGlobalMemorySlots(CHANNEL_TAG, PRODUCER_KEY, {coordinationBuffer});

 // Synchronizing so that all actors have finished registering their memory slots
 MPIbackend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = MPIbackend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto producer = HiCR::ProducerChannel(MPIbackend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating a send slot to put the values we want to communicate
 ELEMENT_TYPE sendBuffer = 42;
 auto sendBufferPtr = &sendBuffer;
 auto sendSlot = MPIbackend->registerLocalMemorySlot(sendBufferPtr, sizeof(ELEMENT_TYPE));

 // Pushing value to the channel
 producer.push(sendSlot);

 // Printing values
 printf("Sent Value:     %u\n", *sendBufferPtr);

 // Make sure the consumer got the message before exiting
 while(producer.getDepth() > 0) producer.checkReceiverPops();

 // Freeing up memory
 MPIbackend->deregisterLocalMemorySlot(sendSlot);
}

void consumerFc(HiCR::Backend* MPIbackend)
{
 // Instantiating backend for memory allocation
 HiCR::backend::sequential::Sequential sequentialBackend;

 // Asking backend to check the available resources
 sequentialBackend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = sequentialBackend.getMemorySpaceList();

 // Getting required buffer sizes
 auto tokenBufferSize = HiCR::ConsumerChannel::getTokenBufferSize(sizeof(ELEMENT_TYPE), CAPACITY);

 // Allocating memory slot for the token buffer (required at the consumer side)
 auto tokenBufferAlloc = sequentialBackend.allocateLocalMemorySlot(*memSpaces.begin(), tokenBufferSize);

 // Getting local pointer of token buffer
 auto tokenBufferPtr = sequentialBackend.getLocalMemorySlotPointer(tokenBufferAlloc);

 // Registering token buffer as MPI memory slot
 auto tokenBuffer = MPIbackend->registerLocalMemorySlot(tokenBufferPtr, tokenBufferSize);

 // Registering buffers globally for them to be used by remote actors
 MPIbackend->exchangeGlobalMemorySlots(CHANNEL_TAG, CONSUMER_KEY, {tokenBuffer});

 // Synchronizing so that all actors have finished registering their memory slots
 MPIbackend->fence(CHANNEL_TAG);

 // Obtaining the globally exchanged memory slots
 auto globalBuffers = MPIbackend->getGlobalMemorySlots()[CHANNEL_TAG];

 // Creating producer and consumer channels
 auto consumer = HiCR::ConsumerChannel(MPIbackend, globalBuffers[CONSUMER_KEY][0], globalBuffers[PRODUCER_KEY][0], sizeof(ELEMENT_TYPE), CAPACITY);

 // Getting the value from the channel
 auto recvSlot = consumer.peekWait();

 // Calculating pointer to the value
 auto recvBuffer = (ELEMENT_TYPE*)MPIbackend->getLocalMemorySlotPointer(tokenBuffer);

 // Printing values
 printf("Received Value: %u\n", recvBuffer[recvSlot[0]]);

 // Popping received value to free up channel
 consumer.pop();

 // Freeing up memory
 sequentialBackend.freeLocalMemorySlot(tokenBuffer);
}

int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Getting MPI values
 int rankCount = 0;
 int rankId = 0;
 MPI_Comm_rank(MPI_COMM_WORLD, &rankId);
 MPI_Comm_size(MPI_COMM_WORLD, &rankCount);

 // Sanity Check
 if (rankCount != 2)
 {
  if(rankId == 0) fprintf(stderr, "Launch error: MPI process count must be 2\n");
  return MPI_Finalize();
 }

 // Instantiating backend
 HiCR::backend::mpi::MPI backend(MPI_COMM_WORLD);

 // Rank 0 is producer, Rank 1 is consumer
 if (rankId == 0) producerFc(&backend);
 if (rankId == 1) consumerFc(&backend);

 return MPI_Finalize();
}

