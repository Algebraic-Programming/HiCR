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

#include <hicr/core/topology.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>
#include "common.hpp"

void coordinatorFc(HiCR::frontend::RPCEngine &rpcEngine)
{
  // Getting instance manager from the rpc engine
  auto im = rpcEngine.getInstanceManager();

  // Querying instance list
  auto &instances = im->getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = im->getCurrentInstance();

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId()) rpcEngine.requestRPC(*instance, TOPOLOGY_RPC_NAME);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances)
    if (instance != coordinator)
    {
      // Getting return value as a memory slot
      auto returnValue = rpcEngine.getReturnValue(*instance);

      // Receiving raw serialized topology information from the worker
      std::string serializedTopology = (char *)returnValue->getPointer();

      // Parsing serialized raw topology into a json object
      auto topologyJson = nlohmann::json::parse(serializedTopology);

      // Freeing return value
      rpcEngine.getMemoryManager()->freeLocalMemorySlot(returnValue);

      // HiCR topology object to obtain
      HiCR::Topology topology;

// Obtaining the topology from the serialized object
#ifdef _HICR_USE_HWLOC_BACKEND_
      topology.merge(HiCR::backend::hwloc::TopologyManager::deserializeTopology(topologyJson));
#endif // _HICR_USE_HWLOC_BACKEND_

#ifdef _HICR_USE_ASCEND_BACKEND_
      topology.merge(HiCR::backend::ascend::TopologyManager::deserializeTopology(topologyJson));
#endif // _HICR_USE_ASCEND_BACKEND_

      // Now summarizing the devices seen by this topology
      printf("* Worker %lu Topology:\n", instance->getId());
      for (const auto &d : topology.getDevices())
      {
        printf("  + '%s'\n", d->getType().c_str());
        printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
        for (const auto &m : d->getMemorySpaceList())
          printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
      }
    }
}
