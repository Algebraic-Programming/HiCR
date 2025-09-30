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

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/hwloc/memoryManager.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>
#include <hicr/backends/pthreads/sharedMemoryFactory.hpp>

#include "../include/consumer.hpp"
#include "../include/producer.hpp"

int main(int argc, char **argv)
{
  // Checking arguments
  if (argc != 2)
  {
    fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
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

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager dm(&topology);

  // Instantiating backend
  HiCR::backend::hwloc::MemoryManager m(&topology);

  // Create shared memory
  auto  sharedMemoryFactory      = HiCR::backend::pthreads::SharedMemoryFactory();
  auto &coordinationSharedMemory = sharedMemoryFactory.get(0, 2);
  auto &payloadSharedMemory      = sharedMemoryFactory.get(1, 2);

  // Set the fence count to the number of threads who are participating in the exchange
  HiCR::backend::pthreads::CommunicationManager consumerCoordinationCommunicationManager(coordinationSharedMemory);
  HiCR::backend::pthreads::CommunicationManager producerCoordinationCommunicationManager(coordinationSharedMemory);
  HiCR::backend::pthreads::CommunicationManager consumerPayloadCommunicationManager(payloadSharedMemory);
  HiCR::backend::pthreads::CommunicationManager producerPayloadCommunicationManager(payloadSharedMemory);

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting a reference to the first memory space
  auto firstMemorySpace = *memSpaces.begin();

  // Rank 0 is producer, Rank 1 is consumer
  auto producerThread = std::thread(producerFc,
                                    std::ref(m),
                                    std::ref(m),
                                    std::ref(producerCoordinationCommunicationManager),
                                    std::ref(producerPayloadCommunicationManager),
                                    firstMemorySpace,
                                    firstMemorySpace,
                                    channelCapacity);
  auto consumerThread = std::thread(consumerFc,
                                    std::ref(m),
                                    std::ref(m),
                                    std::ref(consumerCoordinationCommunicationManager),
                                    std::ref(consumerPayloadCommunicationManager),
                                    firstMemorySpace,
                                    firstMemorySpace,
                                    channelCapacity);

  // Wait for the execution to terminate
  producerThread.join();
  consumerThread.join();

  return 0;
}
