/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dispatcher.cpp
 * @brief Unit tests for the dispatcher class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/L1/tasking/dispatcher.hpp>
#include <hicr/L1/tasking/task.hpp>

TEST(Dispatcher, Construction)
{
  // Creating by context
  EXPECT_NO_THROW(HiCR::L1::tasking::Dispatcher([]()
                                                { return (HiCR::L1::tasking::Task *)NULL; }));

  // Creating by new
  HiCR::L1::tasking::Dispatcher *d = NULL;
  EXPECT_NO_THROW(d = new HiCR::L1::tasking::Dispatcher([]()
                                                        { return (HiCR::L1::tasking::Task *)NULL; }));
  delete d;
}

TEST(Dispatcher, Pull)
{
  HiCR::L1::tasking::Dispatcher d([]()
                                  { return (HiCR::L1::tasking::Task *)NULL; });

  HiCR::L1::tasking::Task *t = NULL;
  EXPECT_NO_THROW(t = d.pull());
  EXPECT_EQ(t, (HiCR::L1::tasking::Task *)NULL);
}
