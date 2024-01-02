#include "include/abcTasks.hpp"
#include <backends/sharedMemory/L1/computeManager.hpp>
#include <backends/sharedMemory/L1/deviceManager.hpp>
#include <hwloc.h>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

// Initializing Sequential backend's device manager
  HiCR::backend::sharedMemory::L1::DeviceManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing Pthreads backend to run in parallel
  HiCR::backend::sharedMemory::L1::ComputeManager computeManager;

  // Running ABCtasks example
  abcTasks(&computeManager, computeResources);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
