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
#include <unordered_map>
#include <CL/opencl.hpp>
#include <hicr/backends/opencl/memoryManager.hpp>
#include <hicr/backends/opencl/topologyManager.hpp>
#include <hicr/backends/opencl/communicationManager.hpp>
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
  const auto                                hostTopology = hostDeviceManager.queryTopology();
  auto                                      hostDevice   = *hostTopology.getDevices().begin();

  // Getting access to the host memory space
  auto hostMemorySpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing opencl topology manager
  HiCR::backend::opencl::TopologyManager openclTopologyManager;
  const auto                                 deviceTopology = openclTopologyManager.queryTopology();
  auto                                       devices        = deviceTopology.getDevices();

  std::vector<cl::Device> openclDevices;
  for (const auto &d : devices)
  {
    auto od = dynamic_pointer_cast<HiCR::backend::opencl::Device>(d);
    openclDevices.push_back(od->getOpenCLDevice());
  }

  // Create a context to work with all OpenCL devices;
  cl::Context context(openclDevices);

  std::unordered_map<HiCR::backend::opencl::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> deviceQueueMap;
  for (const auto &d : devices)
  {
    auto od                     = dynamic_pointer_cast<HiCR::backend::opencl::Device>(d);
    deviceQueueMap[od->getId()] = std::make_shared<cl::CommandQueue>(context, od->getOpenCLDevice());
  }

  // Getting access to all OpenCL devices memory spaces
  std::vector<std::shared_ptr<HiCR::MemorySpace>> openclMemorySpaces;
  for (const auto &d : devices)
    for (const auto &m : d->getMemorySpaceList()) openclMemorySpaces.push_back(m);

  // Define the order of mem spaces for the telephone game
  std::vector<std::shared_ptr<HiCR::MemorySpace>> memSpaceOrder;
  memSpaceOrder.emplace_back(hostMemorySpace);
  memSpaceOrder.insert(memSpaceOrder.end(), openclMemorySpaces.begin(), openclMemorySpaces.end());
  memSpaceOrder.emplace_back(hostMemorySpace);

  // Instantiating Ascend memory and communication managers
  HiCR::backend::opencl::MemoryManager        openclMemoryManager(deviceQueueMap);
  HiCR::backend::opencl::CommunicationManager openclCommunicationManager(deviceQueueMap);

  // Allocate and populate input memory slot
  auto input = openclMemoryManager.allocateLocalMemorySlot(hostMemorySpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(openclMemoryManager, openclCommunicationManager, input, memSpaceOrder, 3);

  // Free input memory slot
  openclMemoryManager.freeLocalMemorySlot(input);

  // Finalize hwloc topology
  hwloc_topology_destroy(topology);

  return 0;
}
