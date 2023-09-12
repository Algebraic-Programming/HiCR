/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.cpp
 * @brief Unit tests for the sequential process class
 * @author S. M. Martin
 * @date 12/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/sequential/process.hpp>
#include <limits>

namespace backend = HiCR::backend::sequential;

TEST(Process, Construction)
{
  backend::Process *p = NULL;

  EXPECT_NO_THROW(p = new backend::Process(0));
  EXPECT_FALSE(p == nullptr);
  delete p;
}

TEST(Process, LifeCycle)
{
  backend::Process p(0);

  // Creating runner function
  auto fc = [&p]()
  {
    // Suspending initially
    p.suspend();

    // Terminating
    p.terminate();
  };

  // Testing forbidden transitions
  EXPECT_THROW(p.start(fc), HiCR::RuntimeException);
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
  EXPECT_NO_THROW(p.start(fc));

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Resuming to terminate function
  EXPECT_NO_THROW(p.resume());

  // Testing forbidden transitions
  EXPECT_THROW(p.initialize(), HiCR::RuntimeException);
  EXPECT_THROW(p.start(fc), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);

  // Awaiting termination
  EXPECT_NO_THROW(p.await());
  EXPECT_THROW(p.start(fc), HiCR::RuntimeException);
  EXPECT_THROW(p.resume(), HiCR::RuntimeException);
  EXPECT_THROW(p.suspend(), HiCR::RuntimeException);
  EXPECT_THROW(p.terminate(), HiCR::RuntimeException);
  EXPECT_THROW(p.await(), HiCR::RuntimeException);

  // Reinitializing
  EXPECT_NO_THROW(p.initialize());
}
