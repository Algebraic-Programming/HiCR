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
#include <hicr/backends/sequential/sequential.hpp>
#include <hicr/task.hpp>
#include <hicr/worker.hpp>

TEST(Worker, Construction)
{
  HiCR::Worker *w = NULL;

  EXPECT_NO_THROW(w = new HiCR::Worker());
  EXPECT_FALSE(w == nullptr);
  delete w;
}

TEST(Task, SetterAndGetters)
{
  HiCR::Worker w;

  // Getting empty lists
  EXPECT_TRUE(w.getProcessingUnits().empty());
  EXPECT_TRUE(w.getDispatchers().empty());

  // Now adding something to the lists/sets
  auto d = HiCR::Dispatcher([]()
                            { return (HiCR::Task *)NULL; });

  // Subscribing worker to dispatcher
  w.subscribe(&d);

  // Creating sequential backend
  HiCR::backend::sequential::Sequential backend;

  // Querying backend for its resources
  backend.queryResources();

  // Gathering compute resources from backend
  auto computeResources = backend.getComputeResourceList();

  // Creating processing unit from resource
  auto processingUnit = backend.createProcessingUnit(computeResources[0]);

  // Assigning processing unit to worker
  w.addProcessingUnit(processingUnit);

  // Getting filled lists
  EXPECT_FALSE(w.getProcessingUnits().empty());
  EXPECT_FALSE(w.getDispatchers().empty());
}

TEST(Worker, LifeCycle)
{
  HiCR::Worker w;

  // Worker state should in an uninitialized state first
  EXPECT_EQ(w.getState(), HiCR::worker::state_t::uninitialized);

  // Attempting to run without any assigned resources
  EXPECT_THROW(w.initialize(), HiCR::LogicException);

  // Creating sequential backend
  HiCR::backend::sequential::Sequential backend;

  // Querying backend for its resources
  backend.queryResources();

  // Gathering compute resources from backend
  auto computeResources = backend.getComputeResourceList();

  // Creating processing unit from resource
  auto processingUnit = backend.createProcessingUnit(computeResources[0]);

  // Assigning processing unit to worker
  w.addProcessingUnit(processingUnit);

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
  EXPECT_EQ(w.getState(), HiCR::worker::state_t::ready);

  // Flag to check running state
  bool runningStateFound = false;

  // Creating task function
  auto f = [&runningStateFound](void *arg)
  {
    // Getting worker pointer
    auto w = HiCR::getCurrentWorker();

    // Getting worker pointer
    auto t = HiCR::getCurrentTask();

    // Checking running state
    if (w->getState() == HiCR::worker::state_t::running) runningStateFound = true;

    // suspending worker and yielding task
    w->suspend();
    t->yield();

    // Terminating worker and yielding task
    w->terminate();
    t->yield();
  };

  // Creating task to run, and setting function to run
  HiCR::Task t(f);

  // Creating task dispatcher
  auto d = HiCR::Dispatcher([&t]()
                            { return &t; });

  // Suscribing worker to dispatcher
  EXPECT_NO_THROW(w.subscribe(&d));

  // Starting worker
  EXPECT_FALSE(runningStateFound);
  EXPECT_NO_THROW(w.start());
  EXPECT_TRUE(runningStateFound);

  // Checking the worker is suspended
  EXPECT_EQ(w.getState(), HiCR::worker::state_t::suspended);

  // Fail on trying to terminate when not running
  EXPECT_THROW(w.terminate(), HiCR::RuntimeException);

  // Testing resume function
  EXPECT_NO_THROW(w.resume());

  // Checking the worker is terminating
  EXPECT_EQ(w.getState(), HiCR::worker::state_t::terminating);

  // Awaiting for worker termination
  EXPECT_NO_THROW(w.await());

  // Checking the worker is terminated
  EXPECT_EQ(w.getState(), HiCR::worker::state_t::terminated);
}
