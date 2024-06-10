#include <chrono>
#include <cstdio>
#include <hwloc.h>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "fibonacci.hpp"

CREATE_HICR_TASKING_HOOK;

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 2)
  {
    fprintf(stderr, "Error: Must provide the fibonacci number to calculate.\n");
    exit(0);
  }

  // Reading argument
  uint64_t initialValue = std::atoi(argv[1]);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Running ABCtasks example
  auto startTime   = std::chrono::high_resolution_clock::now();
  auto result      = fibonacciDriver(initialValue, &computeManager, computeResources);
  auto endTime     = std::chrono::high_resolution_clock::now();
  auto computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

  // Printing result
  printf("Fib(%lu) = %lu. Time: %0.5fs\n", initialValue, result, computeTime.count());

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
