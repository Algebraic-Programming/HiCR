/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file pthreads.cpp
 * @brief Unit tests for the HiCR Pthreads backend
 * @author S. M. Martin
 * @date 21/8/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/sharedMemory/sharedMemory.hpp>
#include <taskr/runtime.hpp>

namespace
{
TEST(Runtime, Construction)
{
  auto t = new HiCR::backend::sharedMemory::SharedMemory();
  auto r = new taskr::Runtime(t);
  EXPECT_FALSE(r == nullptr);
}
} // namespace
