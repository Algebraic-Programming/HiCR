/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file task.hpp
 * @brief Unit tests for the task class
 * @author S. M. Martin
 * @date 21/8/2023
 */

#include "gtest/gtest.h"
#include <hicr/task.hpp>

namespace
{

TEST(Task, Construction)
{
  HiCR::Task *t = new HiCR::Task();
  EXPECT_FALSE(t == nullptr);
}

} // namespace
