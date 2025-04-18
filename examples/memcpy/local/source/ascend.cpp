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
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/topologyManager.hpp>
#include <hicr/backends/ascend/communicationManager.hpp>
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

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  HiCR::backend::ascend::TopologyManager ascendTopologyManager;
  const auto                             deviceTopology = ascendTopologyManager.queryTopology();
  auto                                   ascendDevices  = deviceTopology.getDevices();

  // Getting access to all Ascend devices memory spaces
  std::vector<std::shared_ptr<HiCR::MemorySpace>> ascendMemorySpaces;
  for (const auto &d : ascendDevices)
    for (const auto &m : d->getMemorySpaceList()) ascendMemorySpaces.push_back(m);

  // Define the order of mem spaces for the telephone game
  std::vector<std::shared_ptr<HiCR::MemorySpace>> memSpaceOrder;
  memSpaceOrder.emplace_back(hostMemorySpace);
  memSpaceOrder.insert(memSpaceOrder.end(), ascendMemorySpaces.begin(), ascendMemorySpaces.end());
  memSpaceOrder.emplace_back(hostMemorySpace);

  // Instantiating Ascend memory and communication managers
  HiCR::backend::ascend::MemoryManager        ascendMemoryManager;
  HiCR::backend::ascend::CommunicationManager ascendCommunicationManager;

  // Allocate and populate input memory slot
  auto input = ascendMemoryManager.allocateLocalMemorySlot(hostMemorySpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(ascendMemoryManager, ascendCommunicationManager, input, memSpaceOrder, 3);

  // Free input memory slot
  ascendMemoryManager.freeLocalMemorySlot(input);

  // Finalize ACL
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  return 0;
}
