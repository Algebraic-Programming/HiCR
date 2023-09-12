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
#include <limits>
#include <hicr/backends/sequential/process.hpp>

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

  // Initializing
  EXPECT_NO_THROW(p.initialize());

  // Trying to resume at this point should trigger an exception
  EXPECT_ANY_THROW(p.resume());

  // Creating runner function
  auto fc = [&p]()
  {
   // Suspending initially
   p.suspend();
  };

  // Running
  EXPECT_NO_THROW(p.start(fc));

  // Resuming
  EXPECT_NO_THROW(p.resume());
}
