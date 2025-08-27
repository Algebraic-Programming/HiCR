/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vector>
#include <acl/acl.h>
#include <hicr/backends/acl/memoryManager.hpp>
#include <hicr/backends/acl/topologyManager.hpp>
#include <hicr/backends/acl/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "include/telephoneGame.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager hostDeviceManager(&topology);
  const auto                            hostTopology = hostDeviceManager.queryTopology();
  auto                                  hostDevice   = *hostTopology.getDevices().begin();

  // Getting access to the host memory space
  auto hostMemorySpace = *hostDevice->getMemorySpaceList().begin();

  // Initialize ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize acl. Error %d", err);

  // Initializing acl topology manager
  HiCR::backend::acl::TopologyManager aclTopologyManager;
  const auto                             deviceTopology = aclTopologyManager.queryTopology();
  auto                                   aclDevices  = deviceTopology.getDevices();

  // Getting access to all Huawei devices memory spaces
  std::vector<std::shared_ptr<HiCR::MemorySpace>> aclMemorySpaces;
  for (const auto &d : aclDevices)
    for (const auto &m : d->getMemorySpaceList()) aclMemorySpaces.push_back(m);

  // Define the order of mem spaces for the telephone game
  std::vector<std::shared_ptr<HiCR::MemorySpace>> memSpaceOrder;
  memSpaceOrder.emplace_back(hostMemorySpace);
  memSpaceOrder.insert(memSpaceOrder.end(), aclMemorySpaces.begin(), aclMemorySpaces.end());
  memSpaceOrder.emplace_back(hostMemorySpace);

  // Instantiating acl memory and communication managers
  HiCR::backend::acl::MemoryManager        aclMemoryManager;
  HiCR::backend::acl::CommunicationManager aclCommunicationManager;

  // Allocate and populate input memory slot
  auto input = aclMemoryManager.allocateLocalMemorySlot(hostMemorySpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(aclMemoryManager, aclCommunicationManager, input, memSpaceOrder, 3);

  // Free input memory slot
  aclMemoryManager.freeLocalMemorySlot(input);

  // Finalize ACL
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize acl. Error %d", err);

  return 0;
}
