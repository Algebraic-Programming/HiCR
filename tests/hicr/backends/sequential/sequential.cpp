/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.cpp
 * @brief Unit tests for the sequential backend class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/sequential/sequential.hpp>
#include <limits>

namespace backend = HiCR::backend::sequential;

TEST(Sequential, Construction)
{
  backend::Sequential *b = NULL;

  EXPECT_NO_THROW(b = new backend::Sequential());
  EXPECT_FALSE(b == nullptr);
  delete b;
}

TEST(Sequential, Memory)
{
  backend::Sequential b;

  // Querying resources
  EXPECT_NO_THROW(b.queryMemorySpaces());

  // Getting memory resource list (should be size 1)
  HiCR::memorySpaceList_t mList;
  EXPECT_NO_THROW(mList = b.getMemorySpaceList());
  EXPECT_EQ(mList.size(), 1);

  // Getting memory resource
  auto &r = *mList.begin();

  // Getting total memory size
  size_t testMemAllocSize = 1024;
  size_t totalMem = 0;
  EXPECT_NO_THROW(totalMem = b.getMemorySpaceSize(r));

  // Making sure the system has enough memory for the next test
  EXPECT_GE(totalMem, testMemAllocSize);

  // Trying to allocate more than allowed
  EXPECT_THROW(b.allocateMemorySlot(r, std::numeric_limits<ssize_t>::max()), HiCR::common::LogicException);

  // Allocating memory correctly now
  HiCR::memorySlotId_t s1 = 0;
  EXPECT_NO_THROW(s1 = b.allocateMemorySlot(r, testMemAllocSize));
  EXPECT_EQ(b.getMemorySlotSize(s1), testMemAllocSize);

  // Getting local pointer from allocation
  void *s1LocalPtr = NULL;
  EXPECT_NO_THROW(s1LocalPtr = b.getMemorySlotLocalPointer(s1));
  memset(s1LocalPtr, 0, testMemAllocSize);

  // Creating memory slot from a previous allocation
  HiCR::memorySlotId_t s2 = 0;
  EXPECT_NO_THROW(s2 = b.registerMemorySlot(malloc(testMemAllocSize), testMemAllocSize));
  EXPECT_EQ(b.getMemorySlotSize(s2), testMemAllocSize);

  // Getting local pointer from allocation
  void *s2LocalPtr = NULL;
  EXPECT_NO_THROW(s2LocalPtr = b.getMemorySlotLocalPointer(s2));
  memset(s2LocalPtr, 0, testMemAllocSize);

  // Creating message to transmit
  std::string testMessage("Hello, world!");
  memcpy(s1LocalPtr, testMessage.data(), testMessage.size());

  // Copying message from one slot to the other
  HiCR::Backend::tagId_t tag = 0;
  EXPECT_NO_THROW(b.memcpy(s2, 0, s1, 0, testMessage.size(), tag));

  // Force memcpy operation to finish
  EXPECT_NO_THROW(b.fence(tag));

  // Making sure the message was received
  bool sameStrings = true;
  for (size_t i = 0; i < testMemAllocSize; i++)
    if (((const char *)s1LocalPtr)[i] != ((const char *)s2LocalPtr)[i]) sameStrings = false;
  EXPECT_TRUE(sameStrings);

  // Freeing and reregistering memory slots
  EXPECT_NO_THROW(b.freeMemorySlot(s1));
  EXPECT_NO_THROW(b.deregisterMemorySlot(s2));
}
