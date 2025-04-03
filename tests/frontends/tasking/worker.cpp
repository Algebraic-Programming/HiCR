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

/**
 * @file worker.cpp
 * @brief Unit tests for the worker class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include <hicr/frontends/tasking/task.hpp>
#include <hicr/frontends/tasking/worker.hpp>
#include <hicr/frontends/tasking/tasking.hpp>

TEST(Worker, Construction)
{
  HiCR::tasking::Worker                      *w = NULL;
  HiCR::backend::pthreads::L1::ComputeManager m;

  EXPECT_NO_THROW(w = new HiCR::tasking::Worker(&m, &m, []() { return (HiCR::tasking::Task *)NULL; }));
  EXPECT_FALSE(w == nullptr);
  delete w;
}

TEST(Task, SetterAndGetters)
{
  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::pthreads::L1::ComputeManager c;

  // Creating taskr worker
  HiCR::tasking::Worker w(&c, &c, []() { return (HiCR::tasking::Task *)NULL; });

  // Getting empty lists
  EXPECT_TRUE(w.getProcessingUnits().empty());

  // Initializing HWLoc-based host (CPU) topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = c.createProcessingUnit(firstComputeResource);

  // Assigning processing unit to worker
  w.addProcessingUnit(std::move(processingUnit));

  // Getting filled lists
  EXPECT_FALSE(w.getProcessingUnits().empty());
}

TEST(Worker, LifeCycle)
{
  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::pthreads::L1::ComputeManager c;

  // Creating taskr worker
  HiCR::tasking::Worker w(&c, &c, []() { return (HiCR::tasking::Task *)NULL; });

  // Worker state should in an uninitialized state first
  EXPECT_EQ(w.getState(), HiCR::tasking::Worker::state_t::uninitialized);

  // Attempting to run without any assigned resources
  EXPECT_THROW(w.initialize(), HiCR::LogicException);

  // Initializing HWLoc-based host (CPU) topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = c.createProcessingUnit(firstComputeResource);

  // Assigning processing unit to worker
  w.addProcessingUnit(std::move(processingUnit));

  // Fail on trying to start without initializing
  EXPECT_THROW(w.start(), HiCR::RuntimeException);

  // Now the worker has a resource, the initialization shouldn't fail
  EXPECT_NO_THROW(w.initialize());

  // Fail on trying to await without starting
  EXPECT_THROW(w.await(), HiCR::RuntimeException);

  // Fail on trying to resume without starting
  EXPECT_FALSE(w.suspend());

  // Fail on trying to resume without starting
  EXPECT_FALSE(w.resume());

  // Fail on trying to re-initialize
  EXPECT_THROW(w.initialize(), HiCR::RuntimeException);

  // Worker state should be ready now
  EXPECT_EQ(w.getState(), HiCR::tasking::Worker::state_t::ready);
}
