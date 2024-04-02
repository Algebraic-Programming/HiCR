#include <hwloc.h>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "include/topology.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Calling common function for printing the topology
  topologyFc(tm);

  return 0;
}
