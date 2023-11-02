#include <mpi.h>
#include <hicr.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>

int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Creating MPI-based instance manager
 HiCR::backend::mpi::InstanceManager m;

 // Querying instance list
 auto& instances = m.getInstances();

 // Printing instance information
 for (const auto& instance : instances)
 {
  // Getting instance state
  auto state = instance->getState();

  // Printing state
  printf("Instance State: ");
  if (state == HiCR::Instance::state_t::inactive) printf("inactive");
  if (state == HiCR::Instance::state_t::running)  printf("running");
  if (state == HiCR::Instance::state_t::finished) printf("finished");
  printf("\n");
  fflush(stdout);
 }

 // Finalizing MPI
 MPI_Finalize();

 return 0;
}

