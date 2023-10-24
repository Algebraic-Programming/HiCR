#include <hicr.hpp>
#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>
#include <hicr/backends/lpf/memoryManager.hpp>
#include <consumer.hpp>
#include <producer.hpp>


//pasted from a utils.h header
#define CHECK(f...)                                                   \
{                                                                     \
    const lpf_err_t __r = f;                                          \
    if (__r != LPF_SUCCESS) {                                         \
        printf("Error: '%s' [%s:%i]: %i\n",#f,__FILE__,__LINE__,__r); \
        exit(EXIT_FAILURE);                                           \
    }                                                                 \
}

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

void spmd( lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args )
{

    // Capacity must be larger than zero
    int channelCapacity = (* (int *)args.input);
    if (channelCapacity == 0)
    {
        if(pid == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    }

    HiCR::backend::lpf::MemoryManager m(nprocs, pid, lpf);

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
    if (rank == 0) {
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

