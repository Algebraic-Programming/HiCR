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

 ((uint64_t*)coordinationBufferPtr)[0] = 42;

 printf("Producer Value: %lu\n", *(uint64_t*)coordinationBufferPtr);

 // Test memcpy
 MPIbackend->memcpy(globalBuffers[CONSUMER_KEY][0], 0, globalBuffers[PRODUCER_KEY][0], 0, coordinationBufferSize);

 // Fence
 MPIbackend->fence(CHANNEL_TAG);
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

 // Fence
 MPIbackend->fence(CHANNEL_TAG);

 // Fence
 MPIbackend->fence(CHANNEL_TAG);

 // Printing value
 printf("Consumer Value: %lu\n", *(uint64_t*)tokenBufferPtr);
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

