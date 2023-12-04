#include "include/abcTasks.hpp"
#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <hwloc.h>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Pthreads backend to run in parallel
  HiCR::backend::sharedMemory::ComputeManager computeManager(&topology);

  // Running ABCtasks example
  abcTasks(&computeManager);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
