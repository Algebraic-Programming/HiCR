 #include <mpi.h>
#include "include/common.hpp"
#include "include/coordinator.hpp"
#include "include/worker.hpp"
#include <hicr/L1/topologyManager.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <backends/mpi/L1/instanceManager.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
#include <backends/ascend/L1/topologyManager.hpp>
#endif

#ifdef _HICR_USE_HWLOC_BACKEND_
#include <backends/host/hwloc/L1/topologyManager.hpp>
#endif

int main(int argc, char *argv[])
{
  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Creating MPI-based instance manager
  HiCR::backend::mpi::L1::InstanceManager instanceManager(MPI_COMM_WORLD);

  // Get the locally running instance
  auto myInstance = instanceManager.getCurrentInstance();

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    if (myInstance->isRootInstance()) fprintf(stderr, "Launch error. No machine model file provided\n");
    instanceManager.finalize();
    return -1;
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);
  
  /////////////////////////// Detecting topology managers

  // Clearing current topology managers (if any)
  std::vector<std::unique_ptr<HiCR::L1::TopologyManager>> topologyManagers;

  // Detecting Hwloc
  #ifdef _HICR_USE_HWLOC_BACKEND_
  {
    // Creating HWloc topology object
    auto topology = new hwloc_topology_t;

    // Reserving memory for hwloc
    hwloc_topology_init(topology);

    // Initializing HWLoc-based host (CPU) topology manager
    auto tm = std::make_unique<HiCR::backend::host::hwloc::L1::TopologyManager>(topology);

    // Adding topology manager to the list
    topologyManagers.push_back(std::move(tm));
  }
  #endif

  // Detecting Ascend
  #ifdef _HICR_USE_ASCEND_BACKEND_

  {
    // Initialize (Ascend's) ACL runtime
    aclError err = aclInit(NULL);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

    // Initializing ascend topology manager
    auto tm = std::make_unique<HiCR::backend::ascend::L1::TopologyManager>();

      // Adding topology manager to the list
    topologyManagers.push_back(std::move(tm));
  }

  #endif

  // Checking at least one topology manager was defined
  if (topologyManagers.empty() == true) HICR_THROW_LOGIC("No suitable backends for topology managers were found.\n");
  std::vector<HiCR::L1::TopologyManager*> topologyManagerPointers;
  for (const auto& tm : topologyManagers) topologyManagerPointers.push_back(tm.get());

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true) coordinatorFc(instanceManager, machineModelFile, topologyManagerPointers);
  if (myInstance->isRootInstance() == false) workerFc(instanceManager, topologyManagerPointers);

  // Finalizing HiCR
  instanceManager.finalize();

  return 0;
}
