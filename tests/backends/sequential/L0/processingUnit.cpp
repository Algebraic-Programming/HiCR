/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.cpp
 * @brief Unit tests for the sequential backend processing unit class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include "gtest/gtest.h"
#include <backends/sequential/L0/computeResource.hpp>
#include <backends/sequential/L0/processingUnit.hpp>
#include <backends/sequential/L1/computeManager.hpp>
#include <backends/sequential/L1/topologyManager.hpp>

namespace backend = HiCR::backend::sequential;

TEST(ProcessingUnit, Construction)
{
  backend::L0::ProcessingUnit *p = NULL;

  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting reference to the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  EXPECT_NO_THROW(p = new backend::L0::ProcessingUnit(firstComputeResource));
  EXPECT_FALSE(p == nullptr);
  delete p;
}

TEST(ProcessingUnit, LifeCycle)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Getting first compute resource
  auto computeResource = *computeResources.begin();

  // Creating processin gunit
  backend::L0::ProcessingUnit p(computeResource);

  // Checking that the correct resourceId was used
  std::shared_ptr<HiCR::L0::ComputeResource> pIdAlt = NULL;
  EXPECT_NO_THROW(pIdAlt = p.getComputeResource());
  EXPECT_EQ(pIdAlt, computeResource);

  // Counter for execution times
  int executionTimes = 0;

  // Creating runner function
  auto fc = [&p, &executionTimes]()
  {
    // Increasing execution counter
    executionTimes++;

    // Suspending initially
    p.suspend();

    // Terminating
    p.terminate();
  };

  // Creating compute manager
  HiCR::backend::sequential::L1::ComputeManager m;

  // Creating execution unit
  auto executionUnit = m.createExecutionUnit(fc);

  // Creating execution state
  auto executionState = m.createExecutionState(executionUnit);

  // Testing forbidden transitions
  EXPECT_THROW(p.start(std::move(executionState)), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Initializing
  EXPECT_NO_THROW(p.initialize());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Running
  executionState = m.createExecutionState(executionUnit);
  EXPECT_NO_THROW(p.start(std::move(executionState)));

  // Waiting for execution times to update
  EXPECT_EQ(executionTimes, 1);

  // Testing forbidden transitions
  executionState = m.createExecutionState(executionUnit);
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(std::move(executionState)), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Resuming to terminate function
  EXPECT_NO_THROW(p.resume());

  // Testing forbidden transitions
  executionState = m.createExecutionState(executionUnit);
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(std::move(executionState)), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Awaiting termination
  EXPECT_NO_THROW(p.await());
  executionState = m.createExecutionState(executionUnit);
  EXPECT_THROW(p.start(std::move(executionState)), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Re-running
  auto executionState2 = m.createExecutionState(executionUnit);
  EXPECT_NO_THROW(p.start(std::move(executionState2)));

  // Waiting for execution times to update
  EXPECT_EQ(executionTimes, 2);

  // Re-resuming
  EXPECT_NO_THROW(p.resume());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());
}
