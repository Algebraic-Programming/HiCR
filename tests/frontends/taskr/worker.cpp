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
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>
#include <backends/sharedMemory/pthreads/L1/computeManager.hpp>
#include <frontends/taskr/task.hpp>
#include <frontends/taskr/worker.hpp>

TEST(Worker, Construction)
{
  taskr::Worker *w = NULL;
  HiCR::L1::ComputeManager *m1 = NULL;
  HiCR::backend::sharedMemory::pthreads::L1::ComputeManager m2;

  EXPECT_THROW(w = new taskr::Worker(m1), HiCR::LogicException);
  EXPECT_NO_THROW(w = new taskr::Worker(&m2));
  EXPECT_FALSE(w == nullptr);
  delete w;
}

TEST(Task, SetterAndGetters)
{
  // Instantiating default compute manager
  HiCR::backend::sharedMemory::pthreads::L1::ComputeManager m;

  // Creating taskr worker
  taskr::Worker w(&m);

  // Getting empty lists
  EXPECT_TRUE(w.getProcessingUnits().empty());
  EXPECT_TRUE(w.getDispatchers().empty());

  // Now adding something to the lists/sets
  auto dispatcher = taskr::Dispatcher([]()
                                      { return (taskr::Task *)NULL; });

  // Subscribing worker to dispatcher
  w.subscribe(&dispatcher);

  // Initializing HWLoc backend's topology manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(firstComputeResource);

  // Assigning processing unit to worker
  w.addProcessingUnit(std::move(processingUnit));

  // Getting filled lists
  EXPECT_FALSE(w.getProcessingUnits().empty());
  EXPECT_FALSE(w.getDispatchers().empty());
}

TEST(Worker, LifeCycle)
{
  // Instantiating default compute manager
  HiCR::backend::sharedMemory::pthreads::L1::ComputeManager m;

  // Creating taskr worker
  taskr::Worker w(&m);

  // Worker state should in an uninitialized state first
  EXPECT_EQ(w.getState(), taskr::Worker::state_t::uninitialized);

  // Attempting to run without any assigned resources
  EXPECT_THROW(w.initialize(), HiCR::LogicException);

  // Initializing HWLoc backend's device manager
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(firstComputeResource);

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
  EXPECT_EQ(w.getState(), taskr::Worker::state_t::ready);

  // Flag to check running state
  __volatile__ bool runningStateFound = false;

  // Creating task function
  auto f = [&runningStateFound]()
  {
    // Getting worker pointer
    auto w = taskr::Worker::getCurrentWorker();

    // Checking running state
    if (w->getState() == taskr::Worker::state_t::running) runningStateFound = true;

    // suspending worker and yielding task
    w->suspend();
  };

  // Creating execution unit
  auto u = m.createExecutionUnit(f);

  // Creating task to run, and setting function to run
  taskr::Task t(0, u);

  // Creating task dispatcher
  auto dispatcher = taskr::Dispatcher([&t]()
                                      { return &t; });

  // Suscribing worker to dispatcher
  EXPECT_NO_THROW(w.subscribe(&dispatcher));

  // Starting worker
  EXPECT_FALSE(runningStateFound);
  ASSERT_NO_THROW(w.start());
  while (w.getState() != taskr::Worker::state_t::suspended)
    ;
  EXPECT_TRUE(runningStateFound);

  // Checking the worker is suspended
  EXPECT_EQ(w.getState(), taskr::Worker::state_t::suspended);

  // Fail on trying to terminate when not running
  EXPECT_THROW(w.terminate(), HiCR::RuntimeException);

  // Testing resume function
  EXPECT_NO_THROW(w.resume());

  // Fail on trying to terminate when not running
  EXPECT_NO_THROW(w.terminate());

  // Checking the worker is terminating
  EXPECT_EQ(w.getState(), taskr::Worker::state_t::terminating);

  // Awaiting for worker termination
  EXPECT_NO_THROW(w.await());

  // Checking the worker is terminated
  EXPECT_EQ(w.getState(), taskr::Worker::state_t::terminated);
}
