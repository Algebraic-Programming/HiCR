#include <mpi.h>
#include <hicr.hpp>
#include <hicr/backends/sequential/computeManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>

#define TEST_RPC_PROCESSING_UNIT_ID 0
#define TEST_RPC_EXECUTION_UNIT_ID 0

void coordinatorFc(HiCR::backend::InstanceManager& instanceManager)
{
 // Querying instance list
 auto& instances = instanceManager.getInstances();

 // Printing instance information
 for (const auto& entry : instances)
 {
  // Getting instance index
  auto index = entry.first;

  // Getting instance pointer
  auto instance = entry.second;

  // Getting instance state
  auto state = instance->getState();

  // Printing state
  printf("Instance %lu - State: ", index);
  if (state == HiCR::Instance::state_t::listening)
  {
   printf("listening");
   instance->invoke(TEST_RPC_PROCESSING_UNIT_ID, TEST_RPC_EXECUTION_UNIT_ID);
  }
  if (state == HiCR::Instance::state_t::running)  printf("running");
  if (state == HiCR::Instance::state_t::finished) printf("finished");
  printf("\n");
  fflush(stdout);
 }

 // Finalizing MPI
 MPI_Finalize();
}

void workerFc(HiCR::backend::InstanceManager& instanceManager)
{
 // Initializing sequential backend
 HiCR::backend::sequential::ComputeManager computeManager;

 auto fcLambda = []() { printf("Hello, World!\n");};

 // Creating execution unit
 auto executionUnit = computeManager.createExecutionUnit(fcLambda);

 // Querying compute resources
 computeManager.queryComputeResources();

 // Getting compute resources
 auto computeResources = computeManager.getComputeResourceList();

 // Creating processing unit from the compute resource
 auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());

 // Initialize processing unit
 processingUnit->initialize();

 // Getting current instance
 auto currentInstance = instanceManager.getCurrentInstance();

 // Assigning processing unit to the instance manager
 currentInstance->addProcessingUnit(TEST_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

 // Assigning processing unit to the instance manager
 currentInstance->addExecutionUnit(TEST_RPC_EXECUTION_UNIT_ID, executionUnit);

 // Listening for requests
 currentInstance->listen();

 // Finalizing MPI
 MPI_Finalize();
}


int main(int argc, char **argv)
{
 // Initializing MPI
 MPI_Init(&argc, &argv);

 // Creating MPI-based instance manager
 HiCR::backend::mpi::InstanceManager instanceManager(MPI_COMM_WORLD);

 // Differentiating between coordinator and worker functions
 if (instanceManager.isCoordinatorInstance()) coordinatorFc(instanceManager); else workerFc(instanceManager);

 return 0;
}

