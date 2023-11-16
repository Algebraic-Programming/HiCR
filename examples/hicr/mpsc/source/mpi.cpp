#include <mpi.h>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <consumer.hpp>
#include <producer.hpp>

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
  if (rankCount < 2)
  {
    if (rankId == 0) fprintf(stderr, "Launch error: MPI process count must be at least 2\n");
    return MPI_Finalize();
  }

  // Checking arguments
  if (argc != 2)
  {
    if (rankId == 0) fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
    return MPI_Finalize();
  }

  // Reading argument
  auto channelCapacity = std::atoi(argv[1]);

  // Capacity must be larger than zero
  if (channelCapacity == 0)
  {
    if (rankId == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    return MPI_Finalize();
  }

 // Instantiating backend
 HiCR::backend::mpi::MemoryManager m(MPI_COMM_WORLD);

 // Asking memory manager to check the available memory spaces
 m.queryMemorySpaces();

 // Calculating the number of producer processes
 size_t producerCount = rankCount - 1;

 // Rank 0 is consumer, the rest are producers
 if (rankId == 0) consumerFc(&m, channelCapacity, producerCount);
 if (rankId >= 1) producerFc(&m, channelCapacity, rankId);

  // Finalizing MPI
  MPI_Finalize();

  return 0;
}


