#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <hicr/backends/lpf/memoryManager.hpp>
#include <hicr/backends/lpf/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

/**
 * #DEFAULT_MEMSLOTS The memory slots used by LPF
 * in lpf_resize_memory_register . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MEMSLOTS 100

/**
 * #DEFAULT_MSGSLOTS The message slots used by LPF
 * in lpf_resize_message_queue . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MSGSLOTS 100

typedef struct argList
{
  int capacity;
  int msgCount;
  int tokenSize;
} argList_t;

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  // Capacity must be larger than zero
  argList_t *argList = (argList_t *)args.input;
  if (argList->capacity == 0)
    if (pid == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");

  // Initializing LPF
  CHECK(lpf_resize_message_queue(lpf, DEFAULT_MSGSLOTS));
  CHECK(lpf_resize_memory_register(lpf, DEFAULT_MEMSLOTS));
  CHECK(lpf_sync(lpf, LPF_SYNC_DEFAULT));

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Creating LPF memory and communication managers
  HiCR::backend::lpf::MemoryManager        m(lpf);
  HiCR::backend::lpf::CommunicationManager c(nprocs, pid, lpf);

  // Getting reference to the first memory space detected
  auto firstMemorySpace = *memSpaces.begin();

  auto   start  = MPI_Wtime();
  size_t rankId = pid;

  // Rank 0 is producer, Rank 1 is consumer
  if (pid == 0) producerFc(m, c, firstMemorySpace, argList->capacity, argList->msgCount, argList->tokenSize);
  if (pid == 1) consumerFc(m, c, firstMemorySpace, argList->capacity, argList->msgCount, argList->tokenSize);

  auto end = MPI_Wtime();

  if (rankId == 0) printf("Time: %lf seconds\n", end - start);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int capacity, msgCount, tokenSize;
  if (rank == 0)
  {
    if (size != 2)
    {
      fprintf(stderr, "Error: Must use 2 processes\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // Checking arguments
    if (argc != 4)
    {
      fprintf(stderr, "Error: Must provide <channel capacity> <message count> <token size in bytes> as arguments.\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // For portability, only read STDIN from process 0
    capacity  = atoi(argv[1]);
    msgCount  = atoi(argv[2]);
    tokenSize = atoi(argv[3]);
  }
  MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&msgCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&tokenSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

  argList_t myArgs;
  myArgs.capacity  = capacity;
  myArgs.msgCount  = msgCount;
  myArgs.tokenSize = tokenSize;
  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  args.input       = &myArgs;
  args.input_size  = sizeof(argList_t);
  args.output      = nullptr;
  args.output_size = 0;
  args.f_size      = 0;
  args.f_symbols   = nullptr;
  lpf_init_t init;
  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));
  MPI_Finalize();

  return 0;
}
