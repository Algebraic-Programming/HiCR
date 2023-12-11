#include "include/consumer.hpp"
#include "include/producer.hpp"
#include <backends/lpf/L1/memoryManager.hpp>
#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  // Capacity must be larger than zero
  int channelCapacity = (*(int *)args.input);
  if (channelCapacity == 0)
    if (pid == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");

  HiCR::backend::lpf::L1::MemoryManager m(nprocs, pid, lpf);

  // Asking memory manager to check the available memory spaces
  m.queryMemorySpaces();

  // Rank 0 is producer, Rank 1 is consumer
  if (pid == 0) producerFc(&m, channelCapacity);
  if (pid == 1) consumerFc(&m, channelCapacity);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int capacity;
  if (rank == 0)
  {
    if (size != 2)
    {
      fprintf(stderr, "Error: Must use 2 processes\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // Checking arguments
    if (argc != 2)
    {
      fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // For portability, only read STDIN from process 0
    capacity = atoi(argv[1]);
  }
  MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);

  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  args.input = &capacity;
  args.input_size = sizeof(int);
  args.output = nullptr;
  args.output_size = 0;
  args.f_size = 0;
  args.f_symbols = nullptr;
  lpf_init_t init;
  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));
  MPI_Finalize();

  return 0;
}
