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
#include <backends/sequential/L1/deviceManager.hpp>
#include <backends/sequential/L1/computeManager.hpp>
#include <hicr/L2/tasking/task.hpp>
#include <hicr/L2/tasking/worker.hpp>

TEST(Worker, Construction)
{
  HiCR::L2::tasking::Worker *w = NULL;
  HiCR::L1::ComputeManager *m = NULL;

  EXPECT_NO_THROW(w = new HiCR::L2::tasking::Worker(m));
  EXPECT_FALSE(w == nullptr);
  delete w;
}

TEST(Task, SetterAndGetters)
{
  // Instantiating default compute manager
  HiCR::backend::sequential::L1::ComputeManager m;

  HiCR::L2::tasking::Worker w(&m);

  // Getting empty lists
  EXPECT_TRUE(w.getProcessingUnits().empty());
  EXPECT_TRUE(w.getDispatchers().empty());

  // Now adding something to the lists/sets
  auto dispatcher = HiCR::L2::tasking::Dispatcher([]()
                                         { return (HiCR::L2::tasking::Task *)NULL; });

  // Subscribing worker to dispatcher
  w.subscribe(&dispatcher);

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(*computeResources.begin());

  // Assigning processing unit to worker
  w.addProcessingUnit(std::move(processingUnit));

  // Getting filled lists
  EXPECT_FALSE(w.getProcessingUnits().empty());
  EXPECT_FALSE(w.getDispatchers().empty());
}

TEST(Worker, LifeCycle)
{
  // Instantiating default compute manager
  HiCR::backend::sequential::L1::ComputeManager m;

  HiCR::L2::tasking::Worker w(&m);

  // Worker state should in an uninitialized state first
  EXPECT_EQ(w.getState(), HiCR::L2::tasking::Worker::state_t::uninitialized);

  // Attempting to run without any assigned resources
  EXPECT_THROW(w.initialize(), HiCR::common::LogicException);

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Creating processing unit from resource
  auto processingUnit = m.createProcessingUnit(*computeResources.begin());

  // Assigning processing unit to worker
  w.addProcessingUnit(std::move(processingUnit));

  // Fail on trying to start without initializing
  EXPECT_THROW(w.start(), HiCR::common::RuntimeException);

  // Now the worker has a resource, the initialization shouldn't fail
  EXPECT_NO_THROW(w.initialize());

  // Fail on trying to await without starting
  EXPECT_THROW(w.await(), HiCR::common::RuntimeException);

  // Fail on trying to resume without starting
  EXPECT_THROW(w.suspend(), HiCR::common::RuntimeException);

  // Fail on trying to resume without starting
  EXPECT_THROW(w.resume(), HiCR::common::RuntimeException);

  // Fail on trying to re-initialize
  EXPECT_THROW(w.initialize(), HiCR::common::RuntimeException);

  // Worker state should be ready now
  EXPECT_EQ(w.getState(), HiCR::L2::tasking::Worker::state_t::ready);

  // Flag to check running state
  bool runningStateFound = false;

  // Creating task function
  auto f = [&runningStateFound]()
  {
    // Getting worker pointer
    auto w = HiCR::L2::tasking::Worker::getCurrentWorker();

    // Getting worker pointer
    auto t = HiCR::L2::tasking::Task::getCurrentTask();

    // Checking running state
    if (w->getState() == HiCR::L2::tasking::Worker::state_t::running) runningStateFound = true;

    // suspending worker and yielding task
    w->suspend();
    t->suspend();

    // Terminating worker and yielding task
    w->terminate();
    t->suspend();
  };

  // Creating execution unit
  auto u = m.createExecutionUnit(f);

  // Creating task to run, and setting function to run
  HiCR::L2::tasking::Task t(u);

  // Creating task dispatcher
  auto dispatcher = HiCR::L2::tasking::Dispatcher([&t]()
                                         { return &t; });

  // Suscribing worker to dispatcher
  EXPECT_NO_THROW(w.subscribe(&dispatcher));

  // Starting worker
  EXPECT_FALSE(runningStateFound);
  ASSERT_NO_THROW(w.start());
  EXPECT_TRUE(runningStateFound);

  // Checking the worker is suspended
  EXPECT_EQ(w.getState(), HiCR::L2::tasking::Worker::state_t::suspended);

  // Fail on trying to terminate when not running
  EXPECT_THROW(w.terminate(), HiCR::common::RuntimeException);

  // Testing resume function
  EXPECT_NO_THROW(w.resume());

  // Checking the worker is terminating
  EXPECT_EQ(w.getState(), HiCR::L2::tasking::Worker::state_t::terminating);

  // Awaiting for worker termination
  EXPECT_NO_THROW(w.await());

  // Checking the worker is terminated
  EXPECT_EQ(w.getState(), HiCR::L2::tasking::Worker::state_t::terminated);
}
