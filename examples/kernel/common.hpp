#include <memory>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

/**
 * This function checks on this system for the available compute resources.
 * @return the first compute resource of this system
 */
std::shared_ptr<HiCR::L0::ComputeResource> getFirstComputeResource()
{
  // Creating HWloc topology object
  hwloc_topology_t hwlocTopology;

  // Reserving memory for hwloc
  hwloc_topology_init(&hwlocTopology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::L1::TopologyManager topologyManager(&hwlocTopology);

  // Asking backend to check the available devices
  auto topology = topologyManager.queryTopology();

  // Getting first device found in the topology
  auto device = *topology.getDevices().begin();

  // Getting compute resources
  auto computeResources = device->getComputeResourceList();

  // Selecting the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // give over the compute resource to the launchKernel
  return firstComputeResource;
}
