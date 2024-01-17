#include "include/common.hpp"
#include "include/coordinator.hpp"
#include "include/worker.hpp"

int main(int argc, char *argv[])
{
  // Getting instance manager from the HiCR initialization
  auto instanceManager = HiCR::initialize(&argc, &argv);

  // Get the locally running instance
  auto myInstance = instanceManager->getCurrentInstance();

 // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  auto tm = HiCR::backend::host::hwloc::L1::TopologyManager(&topology);

  // Instantiating host (CPU) memory manager
  auto mm = HiCR::backend::host::hwloc::L1::MemoryManager(&topology);

  // Initializing host (CPU) compute manager manager
  auto km = HiCR::backend::host::pthreads::L1::ComputeManager();

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Selecting a memory space to allocate the required buffers into
  auto bufferMemSpace = *memSpaces.begin();

  // Obtaining compute resources
  auto computeResources = d->getComputeResourceList();

  // Selecting a memory space to allocate the required buffers into
  auto computeResource = *computeResources.begin();

  // Setting memory space for buffer allocations when receiving RPCs
  instanceManager->setBufferMemorySpace(bufferMemSpace);

  // Creating processing unit from the compute resource
  auto processingUnit = km.createProcessingUnit(computeResource);

  // Assigning processing unit to the instance manager
  instanceManager->addProcessingUnit(std::move(processingUnit));

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    if (myInstance->isRootInstance()) fprintf(stderr, "Launch error. No machine model file provided\n");
    HiCR::finalize();
    return -1;
  }
  
  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true)  coordinatorFc(instanceManager, machineModelFile);
  if (myInstance->isRootInstance() == false) workerFc(instanceManager);

  // Finalizing HiCR
  HiCR::finalize();

  return 0;
}
