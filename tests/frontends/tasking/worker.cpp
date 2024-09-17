/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.cpp
 * @brief Unit tests for the worker class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/frontends/tasking/task.hpp>
#include <hicr/frontends/tasking/worker.hpp>
#include <hicr/frontends/tasking/tasking.hpp>

TEST(Worker, Construction)
{
  HiCR::tasking::Worker                            *w  = NULL;
  HiCR::L1::ComputeManager                         *m1 = NULL;
  HiCR::backend::host::pthreads::L1::ComputeManager m2;

  EXPECT_THROW(w = new HiCR::tasking::Worker(m1, []() { return (HiCR::tasking::Task *)NULL; }), HiCR::LogicException);
  EXPECT_NO_THROW(w = new HiCR::tasking::Worker(&m2, []() { return (HiCR::tasking::Task *)NULL; }));
  EXPECT_FALSE(w == nullptr);
  delete w;
}

TEST(Task, SetterAndGetters)
{
  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager c;

  // Creating taskr worker
  HiCR::tasking::Worker w(&c, []() { return (HiCR::tasking::Task *)NULL; });

  // Getting empty lists
  EXPECT_TRUE(w.getProcessingUnits().empty());

  // Initializing HWLoc-based host (CPU) topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

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
  // Initializing HiCR tasking
  HiCR::tasking::initialize();

  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager c;

  // Creating taskr worker
  HiCR::tasking::Worker w(&c, []() { return (HiCR::tasking::Task *)NULL; });

  // Worker state should in an uninitialized state first
  EXPECT_EQ(w.getState(), HiCR::tasking::Worker::state_t::uninitialized);

  // Attempting to run without any assigned resources
  EXPECT_THROW(w.initialize(), HiCR::LogicException);

  // Initializing HWLoc-based host (CPU) topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

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
  EXPECT_THROW(w.suspend(), HiCR::RuntimeException);

  // Fail on trying to resume without starting
  EXPECT_THROW(w.resume(), HiCR::RuntimeException);

  // Fail on trying to re-initialize
  EXPECT_THROW(w.initialize(), HiCR::RuntimeException);

  // Worker state should be ready now
  EXPECT_EQ(w.getState(), HiCR::tasking::Worker::state_t::ready);

  // Finalizing HiCR tasking
  HiCR::tasking::finalize();
}
