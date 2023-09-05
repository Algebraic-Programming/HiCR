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
#include <hicr/backends/sharedMemory/pthreads/pthreads.hpp>

namespace
{
TEST(Pthreads, Construction)
{
  auto t = new HiCR::backend::sharedMemory::pthreads::Pthreads();
  EXPECT_FALSE(t == nullptr);
}
} // namespace
