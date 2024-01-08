#include "include/topology.hpp"
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing backend's device manager
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager t(&topology);

  // Run the topology exchange example
  topologyExchange(t);

  return 0;
}
