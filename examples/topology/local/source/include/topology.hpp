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

#include <hicr/core/L1/topologyManager.hpp>

void topologyFc(HiCR::L1::TopologyManager &topologyManager)
{
  // Querying the devices that this topology manager can detect
  auto topology = topologyManager.queryTopology();

  // Now summarizing the devices seen by this topology manager
  for (const auto &d : topology.getDevices())
  {
    printf("  + '%s'\n", d->getType().c_str());
    printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
    for (const auto &m : d->getMemorySpaceList()) printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
  }
}
