#include "include/abcTasks.hpp"
#include <backends/sequential/L1/computeManager.hpp>
#include <backends/sequential/L1/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Running ABCtasks example
  abcTasks(&computeManager, computeResources);

  return 0;
}
