#include "include/abcTasks.hpp"
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <hwloc.h>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager, computeResources);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
