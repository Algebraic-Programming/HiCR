#include <cstdio>
#include <hwloc.h>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "fibonacci.hpp"

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

  // Checking for maximum fibonacci number to request
  if (initialValue > 30)
  {
    fprintf(stderr, "Error: can only request fibonacci numbers up to 30.\n");
    exit(0);
  }

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Compute resources to use
  HiCR::L0::Device::computeResourceList_t computeResources;

  // Adding all compute resources found
  for (auto &d : t.getDevices())
  {
    // Getting compute resources in this device
    auto cr = d->getComputeResourceList();

    // Adding it to the list
    computeResources.insert(cr.begin(), cr.end());
  }

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Running Fibonacci example
  auto result = fibonacciDriver(initialValue, &computeManager, computeResources);

  // Printing result
  printf("Fib(%lu) = %lu\n", initialValue, result);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
