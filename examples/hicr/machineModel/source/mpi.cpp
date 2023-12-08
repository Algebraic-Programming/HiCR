#include <mpi.h>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/L2/machineModel/builder.hpp>

#define HICR_MACHINE_MODEL_ROOT_INSTANCE_ID 0

int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Creating MPI-based memory manager (necessary for passing data around between instances)
 HiCR::backend::mpi::L1::MemoryManager memoryManager(MPI_COMM_WORLD);

 // Creating MPI-based instance manager (only the coordinator will go beyond this point)
 HiCR::backend::mpi::L1::InstanceManager instanceManager(&memoryManager);

 // Instantiating unified machine model class
 HiCR::L2::machineModel::Builder b(&instanceManager);

 // Obtaining the machine model
 b.build(HICR_MACHINE_MODEL_ROOT_INSTANCE_ID);

 // If this is the root instance, then print the machine model
 printf("%s", b.stringify().c_str());

 // Finalizing MPI
 MPI_Finalize();

 return 0;
}

