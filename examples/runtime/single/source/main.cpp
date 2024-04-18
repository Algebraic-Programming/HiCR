#include <hwloc.h>
#include <hicr/frontends/runtime/runtime.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/host/hwloc/L1/memoryManager.hpp>
#include <hicr/backends/host/pthreads/L1/communicationManager.hpp>
#include "../../common.hpp"

// Worker entry point functions
void entryPointFc(HiCR::Runtime &runtime) { printf("Hello, I am the coordinator itself\n"); };

int main(int argc, char *argv[])
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Using MPI as instance, communication and memory manager for multiple instances
  auto instanceManager      = HiCR::backend::host::L1::InstanceManager::createDefault(&argc, &argv);
  auto communicationManager = std::make_unique<HiCR::backend::host::pthreads::L1::CommunicationManager>();
  auto memoryManager        = std::make_unique<HiCR::backend::host::hwloc::L1::MemoryManager>(&topology);

  // Using HWLoc as topology managers
  std::vector<HiCR::L1::TopologyManager *> topologyManagers;
  auto                                     hwlocTopologyManager = std::make_unique<HiCR::backend::host::hwloc::L1::TopologyManager>(&topology);
  topologyManagers.push_back(hwlocTopologyManager.get());

  // Creating HiCR Runtime
  auto runtime = HiCR::Runtime(instanceManager.get(), communicationManager.get(), memoryManager.get(), topologyManagers);

  // Registering tasks for the workers
  runtime.registerEntryPoint("Coordinator", [&]() { entryPointFc(runtime); });

  // Initializing the HiCR runtime
  runtime.initialize();

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    fprintf(stderr, "Launch error. No machine model file provided\n");
    runtime.abort(-1);
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Loading machine model
  auto machineModel = loadMachineModelFromFile(machineModelFile);

  // Finally, deploying machine model
  runtime.deploy(machineModel, &isTopologyAcceptable);

  // Finalizing runtime
  runtime.finalize();

  return 0;
}
