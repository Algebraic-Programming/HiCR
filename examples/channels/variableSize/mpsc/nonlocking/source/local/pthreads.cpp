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

#include <thread>
#include <vector>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/hwloc/memoryManager.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>
#include <hicr/backends/pthreads/core.hpp>

#include "../include/consumer.hpp"
#include "../include/producer.hpp"

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 3)
  {
    fprintf(stderr, "Error: Must provide the channel capacity and the thread pool size as argument.\n");
    return -1;
  }

  // Reading argument
  auto channelCapacity = std::atoi(argv[1]);

  // Capacity must be larger than zero
  if (channelCapacity == 0)
  {
    fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");
    return -1;
  }

  // Reading argument
  size_t threadPoolSize = std::atoi(argv[2]);

  // Capacity must be larger than zero
  if (threadPoolSize == 0)
  {
    fprintf(stderr, "Error: Cannot create a thread pool with zero capacity.\n");
    return -1;
  }

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager dm(&topology);

  // Instantiating backend
  HiCR::backend::hwloc::MemoryManager m(&topology);

  // Create shared memory
  auto coordinationSharedMemory = HiCR::backend::pthreads::Core(threadPoolSize);
  auto payloadSharedMemory      = HiCR::backend::pthreads::Core(threadPoolSize);

  // Create communication managers
  std::vector<HiCR::backend::pthreads::CommunicationManager> coordinationCommunicationManagers;
  std::vector<HiCR::backend::pthreads::CommunicationManager> payloadCommunicationManagers;
  for (size_t i = 0; i < threadPoolSize; ++i)
  {
    coordinationCommunicationManagers.emplace_back(coordinationSharedMemory);
    payloadCommunicationManagers.emplace_back(payloadSharedMemory);
  }

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  // Create thread pool
  std::vector<std::thread> threadPool;
  for (size_t threadId = 0; threadId < threadPoolSize; ++threadId)
  {
    // The first thread is a consumer
    if (threadId == 0)
    {
      threadPool.emplace_back(consumerFc,
                              std::ref(m),
                              std::ref(m),
                              std::ref(coordinationCommunicationManagers[threadId]),
                              std::ref(payloadCommunicationManagers[threadId]),
                              firstMemorySpace,
                              firstMemorySpace,
                              channelCapacity,
                              threadPoolSize - 1);
    }
    // All the others are producers
    else
    {
      threadPool.emplace_back(producerFc,
                              std::ref(m),
                              std::ref(m),
                              std::ref(coordinationCommunicationManagers[threadId]),
                              std::ref(payloadCommunicationManagers[threadId]),
                              firstMemorySpace,
                              firstMemorySpace,
                              channelCapacity,
                              threadId - 1,
                              threadPoolSize - 1);
    }
  }

  // Wait for the execution to terminate
  for (auto &thread : threadPool) { thread.join(); }

  return 0;
}
