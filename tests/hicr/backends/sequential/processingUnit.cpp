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
#include <hicr/backends/sequential/L0/processingUnit.hpp>
#include <hicr/backends/sequential/L1/computeManager.hpp>

namespace backend = HiCR::backend::sequential;

TEST(ProcessingUnit, Construction)
{
  backend::L0::ProcessingUnit *p = NULL;

  EXPECT_NO_THROW(p = new backend::L0::ProcessingUnit(0));
  EXPECT_FALSE(p == nullptr);
  delete p;
}

TEST(ProcessingUnit, LifeCycle)
{
  HiCR::L0::computeResourceId_t pId = 0;
  backend::L0::ProcessingUnit p(pId);

  // Checking that the correct resourceId was used
  HiCR::L0::computeResourceId_t pIdAlt = pId + 1;
  EXPECT_NO_THROW(pIdAlt = p.getComputeResourceId());
  EXPECT_EQ(pIdAlt, pId);

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

  // Testing forbidden transitions
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::common::RuntimeException);

  // Initializing
  EXPECT_NO_THROW(p.initialize());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::common::RuntimeException);

  // Running
  auto executionState = p.createExecutionState(executionUnit);
  EXPECT_NO_THROW(p.start(std::move(executionState)));

  // Waiting for execution times to update
  EXPECT_EQ(executionTimes, 1);

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);

  // Resuming to terminate function
  EXPECT_NO_THROW(p.resume());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);

  // Awaiting termination
  EXPECT_NO_THROW(p.await());
  EXPECT_THROW(p.start(std::move(p.createExecutionState(executionUnit))), HiCR::common::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::common::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::common::RuntimeException);

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());

  // Re-running
  auto executionState2 = p.createExecutionState(executionUnit);
  EXPECT_NO_THROW(p.start(std::move(executionState2)));

  // Waiting for execution times to update
  EXPECT_EQ(executionTimes, 2);

  // Re-resuming
  EXPECT_NO_THROW(p.resume());

  // Re-awaiting
  EXPECT_NO_THROW(p.await());
}
