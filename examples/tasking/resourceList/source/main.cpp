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

#include <chrono>
#include <cstdio>
#include <hwloc.h>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "../runtime.hpp"
#include "source/workTask.hpp"

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Pthread-base compute manager to run tasks in parallel
  HiCR::backend::pthreads::ComputeManager computeManager;

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting compute resource lists from devices
  std::vector<HiCR::Device::computeResourceList_t> computeResourceLists;
  for (auto d : t.getDevices()) computeResourceLists.push_back(d->getComputeResourceList());

  // Initializing runtime
  Runtime runtime(&computeManager, &computeManager);

  // Getting work task count
  size_t workTaskCount = 100;
  size_t iterations    = 5000;
  if (argc > 1) workTaskCount = std::atoi(argv[1]);
  if (argc > 2) iterations = std::atoi(argv[2]);

  // Getting the core subset from the argument list (could be from a file too)
  std::set<int> coreSubset;
  for (int i = 3; i < argc; i++) coreSubset.insert(std::atoi(argv[i]));

  // Sanity check
  if (coreSubset.empty())
  {
    fprintf(stderr, "Launch error: no compute resources provided\n");
    exit(-1);
  }

  // Create processing units from the detected compute resource list and giving them to runtime
  for (auto computeResourceList : computeResourceLists)
    for (auto computeResource : computeResourceList)
    {
      // Interpreting compute resource as core
      auto core = dynamic_pointer_cast<HiCR::backend::hwloc::ComputeResource>(computeResource);

      // If the core affinity is included in the list, create new processing unit
      if (coreSubset.contains(core->getProcessorId()))
      {
        // Creating a processing unit out of the computational resource
        auto processingUnit = computeManager.createProcessingUnit(computeResource);

        // Assigning resource to the runtime
        runtime.addProcessingUnit(std::move(processingUnit));
      }
    }

  // Creating task  execution unit
  auto taskFc = [&iterations](void *arg) { work(iterations); };

  // Adding multiple compute tasks
  printf("Running %lu work tasks with %lu processing units...\n", workTaskCount, coreSubset.size());
  for (size_t i = 0; i < workTaskCount; i++) runtime.addTask(new Task(i, taskFc));

  // Running runtime only on the core subset
  auto t0 = std::chrono::high_resolution_clock::now();
  runtime.run();
  auto tf = std::chrono::high_resolution_clock::now();

  auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
  printf("Finished in %.3f seconds.\n", (double)dt * 1.0e-9);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
