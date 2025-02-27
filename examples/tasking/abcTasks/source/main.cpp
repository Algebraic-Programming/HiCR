#include <hwloc.h>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include "abcTasks.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::pthreads::L1::ComputeManager computeManager;

  // Initializing runtime
  Runtime runtime(&computeManager, &computeManager);

  // Assigning processing units for the runtime
  for (const auto &computeResource : computeResources) runtime.addProcessingUnit(computeManager.createProcessingUnit(computeResource));

  // Running ABCtasks example
  abcTasks(runtime);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
