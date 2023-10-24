#include <mpi.h>
#include <hicr.hpp>
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
 if (rankCount != 2)
 {
  if(rankId == 0) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
  return MPI_Finalize();
 }

 // Checking arguments
 if (argc != 2)
 {
   if(rankId == 0) fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
   return MPI_Finalize();
 }

 // Reading argument
 auto channelCapacity = std::atoi(argv[1]);

 // Capacity must be larger than zero
 if (channelCapacity == 0)
 {
   if(rankId == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
   return MPI_Finalize();
 }

 // Creating communicator that includes only the producer and the consumer
 // This is not necessary in this example because only two processes run and MPI_COMM_WORLD suffices
 // However, in a real world scenario, this is might be required for preventing involving other ranks in its creation and use
 const int ranks[2] = {0, 1};
 MPI_Group commWorldGroup;
 MPI_Group channelGroup;
 MPI_Comm_group(MPI_COMM_WORLD, &commWorldGroup);
 MPI_Group_incl(commWorldGroup, 2, ranks, &channelGroup);
 MPI_Comm channelCommunicator;
 MPI_Comm_create_group(MPI_COMM_WORLD, channelGroup, 0, &channelCommunicator);

 // Instantiating backend
 HiCR::backend::mpi::MemoryManager m(channelCommunicator);

 // Rank 0 is producer, Rank 1 is consumer
 if (rankId == 0) producerFc(&m, channelCapacity);
 if (rankId == 1) consumerFc(&m, channelCapacity);

 // Finalizing MPI
 MPI_Finalize();

 return 0;
}

