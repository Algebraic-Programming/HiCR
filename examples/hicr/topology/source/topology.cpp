#include <hicr/L1/topologyManager.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
#include <acl/acl.h>
#include <backends/ascend/L1/topologyManager.hpp>
#endif // _HICR_USE_ASCEND_BACKEND_

#ifdef _HICR_USE_HWLOC_BACKEND_
#include <hwloc.h>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#endif // _HICR_USE_HWLOC_BACKEND_

int main(int argc, char **argv)
{
  // Storage to gather all topology managers to use in this example
  std::vector<HiCR::L1::TopologyManager*> topologyManagerList;

  #ifdef _HICR_USE_HWLOC_BACKEND_

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager hwlocTopologyManager(&topology);

  // Adding topology manager to the list
  topologyManagerList.push_back(&hwlocTopologyManager);

  #endif // _HICR_USE_HWLOC_BACKEND_

  #ifdef _HICR_USE_ASCEND_BACKEND_

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;

  // Adding topology manager to the list
  topologyManagerList.push_back(&ascendTopologyManager);

  #endif // _HICR_USE_ASCEND_BACKEND_

  // Printing device list
  printf("Gathering device information from %lu topology manager(s)...\n", topologyManagerList.size());
  printf("Devices: \n");

  // Now iterating over all registered topology managers
  for (auto t : topologyManagerList)
  {
    // Querying devices visible by the passed topology manager
    t->queryDevices();

    // Printing all devices
    for (const auto& d : t->getDevices())
    {
      printf("  + '%s'\n", d->getType().c_str());
      printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
      for (const auto& m : d->getMemorySpaceList())      printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul*1024ul*1024ul));
    }
  }

  return 0;
}
