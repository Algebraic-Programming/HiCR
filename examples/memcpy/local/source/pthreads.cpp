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

#include "include/telephoneGame.hpp"
#include <hicr/backends/hwloc/memoryManager.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager dm(&topology);

  // Instantiating host (CPU) memory manager
  HiCR::backend::hwloc::MemoryManager m(&topology);

  // Instantiating host (CPU) communication manager
  HiCR::backend::pthreads::CommunicationManager c;

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<std::shared_ptr<HiCR::MemorySpace>>(memSpaces.begin(), memSpaces.end());

  // Allocating memory slots in different NUMA domains
  auto firstMemSpace = *memSpaces.begin();
  auto input         = m.allocateLocalMemorySlot(firstMemSpace, BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, c, input, memSpaceOrder, ITERATIONS);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  return 0;
}
