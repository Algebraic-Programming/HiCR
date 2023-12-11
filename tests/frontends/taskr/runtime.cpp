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
#include <frontends/taskr/runtime.hpp>

TEST(Runtime, Construction)
{
  auto r = new taskr::Runtime();
  EXPECT_FALSE(r == nullptr);
}
