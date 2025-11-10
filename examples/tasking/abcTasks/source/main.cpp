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

#include <hwloc.h>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "abcTasks.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::pthreads::ComputeManager computeManager;

  // Initializing runtime
  Runtime runtime(&computeManager, &computeManager);

  // Assigning processing units for the runtime
  for (const auto &computeResource : computeResources) runtime.addProcessingUnit(computeManager.createProcessingUnit(computeResource));

  // Running ABCtasks example
  abcTasks(runtime, &computeManager);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
