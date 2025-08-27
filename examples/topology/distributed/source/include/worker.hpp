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

#pragma once

#include <hicr/core/memorySpace.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/topologyManager.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include "common.hpp"

void sendTopology(HiCR::frontend::RPCEngine &rpcEngine)
{
  // Storage for the topology to send
  HiCR::Topology workerTopology;

  // List of topology managers to query
  std::vector<std::shared_ptr<HiCR::TopologyManager>> topologyManagerList;

// Now instantiating topology managers (which ones is determined by backend availability during compilation)
#ifdef _HICR_USE_HWLOC_BACKEND_

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  auto hwlocTopologyManager = std::make_shared<HiCR::backend::hwloc::TopologyManager>(&topology);

  // Adding topology manager to the list
  topologyManagerList.push_back(hwlocTopologyManager);

#endif // _HICR_USE_HWLOC_BACKEND_

#ifdef _HICR_USE_ACL_BACKEND_

  // Initialize ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize acl. Error %d", err);

  // Initializing acl topology manager
  auto aclTopologyManager = std::make_shared<HiCR::backend::acl::TopologyManager>();

  // Adding topology manager to the list
  topologyManagerList.push_back(aclTopologyManager);

#endif // _HICR_USE_ACL_BACKEND_

  // For each topology manager detected
  for (const auto &tm : topologyManagerList)
  {
    // Getting the topology information from the topology manager
    const auto t = tm->queryTopology();

    // Merging its information to the worker topology object to send
    workerTopology.merge(t);
  }

  // Serializing theworker topology and dumping it into a raw string message
  auto message = workerTopology.serialize().dump();

  // Registering return value
  rpcEngine.submitReturnValue(message.data(), message.size() + 1);
}

void workerFc(HiCR::frontend::RPCEngine &rpcEngine)
{
  // Creating compute manager (responsible for executing the RPC)
  HiCR::backend::pthreads::ComputeManager cpm;

  // Creating execution unit to run as RPC
  auto executionUnit = std::make_shared<HiCR::backend::pthreads::ExecutionUnit>([&](void *closure) { sendTopology(rpcEngine); });

  // Adding RPC target by name and the execution unit id to run
  rpcEngine.addRPCTarget(TOPOLOGY_RPC_NAME, executionUnit);

  // Listening for RPC requests
  rpcEngine.listen();
}
