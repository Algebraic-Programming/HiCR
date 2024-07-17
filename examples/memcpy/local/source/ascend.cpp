#include <vector>
#include <acl/acl.h>
#include <hicr/backends/ascend/L1/memoryManager.hpp>
#include <hicr/backends/ascend/L1/topologyManager.hpp>
#include <hicr/backends/ascend/L1/communicationManager.hpp>
#include <hicr/backends/host/hwloc/L1/memoryManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "include/telephoneGame.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager hostDeviceManager(&topology);
  const auto                                      hostTopology = hostDeviceManager.queryTopology();
  auto                                            hostDevice   = *hostTopology.getDevices().begin();

  // Getting access to the host memory space
  auto hostMemorySpace = *hostDevice->getMemorySpaceList().begin();

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;
  const auto                                 deviceTopology = ascendTopologyManager.queryTopology();
  auto                                       ascendDevices  = deviceTopology.getDevices();

  // Getting access to all ascend devices memory spaces
  std::vector<std::shared_ptr<HiCR::L0::MemorySpace>> ascendMemorySpaces;
  for (const auto &d : ascendDevices)
    for (const auto &m : d->getMemorySpaceList()) ascendMemorySpaces.push_back(m);

  // Define the order of mem spaces for the telephone game
  std::vector<std::shared_ptr<HiCR::L0::MemorySpace>> memSpaceOrder;
  memSpaceOrder.emplace_back(hostMemorySpace);
  memSpaceOrder.insert(memSpaceOrder.end(), ascendMemorySpaces.begin(), ascendMemorySpaces.end());
  memSpaceOrder.emplace_back(hostMemorySpace);

  // Allocate and populate input memory slot
  HiCR::backend::host::hwloc::L1::MemoryManager hostMemoryManager(&topology);
  auto                                          input = hostMemoryManager.allocateLocalMemorySlot(hostMemorySpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Instantiating Ascend memory and communication managers
  HiCR::backend::ascend::L1::MemoryManager        ascendMemoryManager;
  HiCR::backend::ascend::L1::CommunicationManager ascendCommunicationManager;

  // Run the telephone game
  telephoneGame(ascendMemoryManager, ascendCommunicationManager, input, memSpaceOrder, 3);

  // Free input memory slot
  hostMemoryManager.freeLocalMemorySlot(input);

  // Finalize ACL
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  return 0;
}
