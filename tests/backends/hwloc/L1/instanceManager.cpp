/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.cpp
 * @brief Unit tests for the hwloc instance manager backend
 * @author S. M. Martin
 * @date 16/8/2024
 */

#include <limits>
#include "gtest/gtest.h"
#include <hicr/core/L0/instance.hpp>
#include <hicr/backends/hwloc/L1/instanceManager.hpp>

TEST(MemoryManager, InstanceManager)
{
  HiCR::backend::hwloc::L1::InstanceManager *im = NULL;

  EXPECT_NO_THROW(im = new HiCR::backend::hwloc::L1::InstanceManager());
  EXPECT_FALSE(im == nullptr);
  delete im;
}

#define TEST_VALUE 42
TEST(MemoryManager, Lifetime)
{
  auto im = HiCR::backend::hwloc::L1::InstanceManager::createDefault(nullptr, nullptr);

  bool RPCExecuted = false;
  EXPECT_NO_THROW(im->addRPCTarget("Test", [&]() {
    RPCExecuted = true;
    int value   = TEST_VALUE;
    im->submitReturnValue(&value, sizeof(int));
  }));

  auto currentInstance = im->getCurrentInstance();
  EXPECT_TRUE(currentInstance->isRootInstance());
  EXPECT_NO_THROW(im->launchRPC(*currentInstance, "Test"));
  EXPECT_TRUE(RPCExecuted);

  int *returnBuffer = nullptr;
  EXPECT_NO_THROW(returnBuffer = (int *)im->getReturnValue(*currentInstance));
  EXPECT_EQ(*returnBuffer, TEST_VALUE);

  EXPECT_ANY_THROW(im->listen());
  EXPECT_ANY_THROW(im->addInstance(1));
  HiCR::L0::Topology topology;
  EXPECT_ANY_THROW(im->createInstance(topology));

  EXPECT_DEATH(im->abort(1), "");
}
