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

#include <mpi.h>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "include/RPCTest.hpp"

int main(int argc, char **argv)
{
  // Initializing instance manager
  auto im = HiCR::backend::mpi::InstanceManager::createDefault(&argc, &argv);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Creating compute manager (responsible for executing the RPC)
  HiCR::backend::pthreads::ComputeManager cpm;

  // Creating memory and communication managers (buffering and communication)
  HiCR::backend::mpi::MemoryManager        mm;
  HiCR::backend::mpi::CommunicationManager cc;

  // Gathering topology from the topology manager
  const auto t = tm.queryTopology();

  // Selecting first device
  auto d = *t.getDevices().begin();

 // Getting memory space list from device
  auto memSpaces = d->getMemorySpaceList();

  // Grabbing first memory space for buffering
  auto bufferMemorySpace = *memSpaces.begin();

  // Now getting compute resource list from device
  auto computeResources = d->getComputeResourceList();

  // Grabbing first compute resource for computing incoming RPCs
  auto executeResource = *computeResources.begin(); 

  // Creating execution unit to run as RPC
  auto executionUnit =
    std::make_shared<HiCR::backend::pthreads::ExecutionUnit>([&im](void *closure) { printf("Instance %lu: running Test RPC\n", im->getCurrentInstance()->getId()); });

  // Calling common function for printing the topology
  RPCTestFc(cc, mm, cpm, *im, bufferMemorySpace, executeResource, executionUnit);

  // Finalizing instance manager
  im->finalize();

  return 0;
}
