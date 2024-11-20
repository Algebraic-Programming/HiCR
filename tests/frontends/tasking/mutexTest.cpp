/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.cpp
 * @brief Unit tests for the mutex class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include <set>
#include "gtest/gtest.h"
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/frontends/tasking/mutex.hpp>
#include <hicr/frontends/tasking/task.hpp>
#include <hicr/frontends/tasking/worker.hpp>
#include <hicr/frontends/tasking/tasking.hpp>

TEST(Worker, Construction)
{
  HiCR::tasking::Mutex *m = NULL;
  EXPECT_NO_THROW(m = new HiCR::tasking::Mutex());
  EXPECT_FALSE(m == nullptr);
  delete m;
}

TEST(Mutex, LifeTime)
{
  // Creating mutex
  HiCR::tasking::Mutex m;

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Set to hold tasks who received the suspend signal
  std::set<HiCR::tasking::Task *> suspendedTasks;

  // Set to hold tasks who received the sync (resume) signal
  std::set<HiCR::tasking::Task *> syncedTasks;

  // Setting callback map
  HiCR::tasking::Task::taskCallbackMap_t callbackMap;
  callbackMap.setCallback(HiCR::tasking::Task::callback_t::onTaskSuspend, [&](HiCR::tasking::Task *task) { suspendedTasks.insert(task); });
  callbackMap.setCallback(HiCR::tasking::Task::callback_t::onTaskSync, [&](HiCR::tasking::Task *task) { syncedTasks.insert(task); });

  // Creating task function
  auto taskBFc = [&m](void *arg) { m.lock((HiCR::tasking::Task *)arg); };

  // Instantiating Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager c;

  // Creating execution unit
  auto u = c.createExecutionUnit(taskBFc);

  // Creating tasks
  HiCR::tasking::Task taskA(u, &callbackMap);
  HiCR::tasking::Task taskB(u, &callbackMap);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto tp = tm.queryTopology();

  // Getting first device found
  auto d = *tp.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = c.createProcessingUnit(firstComputeResource);

  // Initializing processing unit
  processingUnit->initialize();

  // Creating execution state
  auto executionState = c.createExecutionState(u, &taskB);

  // Then initialize the task with the new execution state
  taskB.initialize(std::move(executionState));

  // Locking with A
  EXPECT_NO_THROW(m.lock((HiCR::tasking::Task *)&taskA));
  EXPECT_TRUE(m.ownsLock((HiCR::tasking::Task *)&taskA));
  EXPECT_FALSE(m.ownsLock((HiCR::tasking::Task *)&taskB));
  EXPECT_NO_THROW(m.unlock((HiCR::tasking::Task *)&taskA));

  // Trying to lock
  EXPECT_TRUE(m.trylock((HiCR::tasking::Task *)&taskA));
  EXPECT_FALSE(m.trylock((HiCR::tasking::Task *)&taskA));
  EXPECT_FALSE(m.trylock((HiCR::tasking::Task *)&taskB));

  // Suspending on lock
  taskB.run();
  EXPECT_FALSE(suspendedTasks.contains(&taskA));
  EXPECT_TRUE(suspendedTasks.contains(&taskB));
  EXPECT_NO_THROW(m.unlock((HiCR::tasking::Task *)&taskA));
  EXPECT_FALSE(syncedTasks.contains(&taskA));
  EXPECT_TRUE(syncedTasks.contains(&taskB));

  EXPECT_NO_THROW(m.unlock((HiCR::tasking::Task *)&taskB));
  EXPECT_ANY_THROW(m.unlock((HiCR::tasking::Task *)&taskA));
}