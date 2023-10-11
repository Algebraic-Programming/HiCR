#include <hicr.hpp>
#include <lpf/core.h>
#include <hicr/backends/lpf/lpf.hpp>
#include <source/consumer.hpp>
#include <source/producer.hpp>

void spmd( lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args )
{

  int channelCapacity =  2; //* (int *) args.input;

  auto backend = new  HiCR::backend::lpf::LpfBackend(nprocs, pid, lpf);
  backend->queryResources();


 // Sanity Check
 if (nprocs != 2)
 {
  if(pid == 0) fprintf(stderr, "Launch error: MPI process count must be equal to 2\n");
 }

 // Reading argument
 //std::cout << "Channel capacity = " << channelCapacity << std::endl;

 // Capacity must be larger than zero
 if (channelCapacity == 0)
 {
   if(pid == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
 }

 // Rank 0 is producer, Rank 1 is consumer
 if (pid == 0) producerFc(backend, channelCapacity);
 if (pid == 1) consumerFc(backend, channelCapacity);

 // Freeing memory
 delete backend;

}

int main(int argc, char **argv)
{

 // Checking arguments
 if (argc != 2)
 {
   fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
   std::abort();
 }

  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  int capacity = atoi(argv[1]);
  std::cout << "Capacity: " << capacity << std::endl;
  args.input = &capacity;
  args.input_size = sizeof(int);
  args.output = nullptr;
  args.output_size = 0;
  args.f_size = 0;
  args.f_symbols = nullptr;
  lpf_err_t rc = lpf_exec( LPF_ROOT, LPF_MAX_P, &spmd, args);
  EXPECT_EQ("%d", LPF_SUCCESS, rc);

  return 0;
}

