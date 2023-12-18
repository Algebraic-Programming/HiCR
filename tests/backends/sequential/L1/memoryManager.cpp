/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorymanager.cpp
 * @brief Unit tests for the sequential backend memory manager class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <backends/sequential/L1/memoryManager.hpp>
#include <backends/sequential/L1/deviceManager.hpp>
#include <hicr/common/exceptions.hpp>
#include <limits>

namespace backend = HiCR::backend::sequential;

TEST(MemoryManager, Construction)
{
  backend::L1::MemoryManager *b = NULL;

  EXPECT_NO_THROW(b = new backend::L1::MemoryManager());
  EXPECT_FALSE(b == nullptr);
  delete b;
}

TEST(MemoryManager, Memory)
{
  // Initializing Sequential backend's device manager
  HiCR::backend::sequential::L1::DeviceManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L0::MemorySpace*>(memSpaces.begin(), memSpaces.end());

  // Instantiating sequential backend's memory manager
  HiCR::backend::sequential::L1::MemoryManager m;

  // Getting memory resource list (should be size 1)
  HiCR::L0::Device::memorySpaceList_t mList;
  EXPECT_NO_THROW(mList = d->getMemorySpaceList());
  EXPECT_EQ(mList.size(), 1);

  // Getting memory resource
  auto &r = *mList.begin();

  // Getting total memory size
  size_t testMemAllocSize = 1024;
  size_t totalMem = 0;
  EXPECT_NO_THROW(totalMem = r->getSize());

  // Making sure the system has enough memory for the next test
  EXPECT_GE(totalMem, testMemAllocSize);

  // Trying to allocate more than allowed
  EXPECT_THROW(m.allocateLocalMemorySlot(r, std::numeric_limits<ssize_t>::max()), HiCR::common::LogicException);

  // Allocating memory correctly now
  HiCR::L0::LocalMemorySlot *s1 = NULL;
  EXPECT_NO_THROW(s1 = m.allocateLocalMemorySlot(r, testMemAllocSize));
  EXPECT_EQ(s1->getSize(), testMemAllocSize);

  // Getting local pointer from allocation
  void *s1LocalPtr = NULL;
  EXPECT_NO_THROW(s1LocalPtr = s1->getPointer());
  memset(s1LocalPtr, 0, testMemAllocSize);

  // Creating memory slot from a previous allocation
  HiCR::L0::LocalMemorySlot *s2 = NULL;
  EXPECT_NO_THROW(s2 = m.registerLocalMemorySlot(r, malloc(testMemAllocSize), testMemAllocSize));
  EXPECT_EQ(s2->getSize(), testMemAllocSize);

  // Getting local pointer from allocation
  void *s2LocalPtr = NULL;
  EXPECT_NO_THROW(s2LocalPtr = s2->getPointer());
  memset(s2LocalPtr, 0, testMemAllocSize);

  // Creating message to transmit
  std::string testMessage("Hello, world!");
  memcpy(s1LocalPtr, testMessage.data(), testMessage.size());

  // Copying message from one slot to the other
  EXPECT_NO_THROW(m.memcpy(s2, 0, s1, 0, testMessage.size()));

  // Force memcpy operation to finish
  EXPECT_NO_THROW(m.fence(0));

  // Making sure the message was received
  bool sameStrings = true;
  for (size_t i = 0; i < testMemAllocSize; i++)
    if (((const char *)s1LocalPtr)[i] != ((const char *)s2LocalPtr)[i]) sameStrings = false;
  EXPECT_TRUE(sameStrings);

  // Freeing and reregistering memory slots
  EXPECT_NO_THROW(m.freeLocalMemorySlot(s1));
  EXPECT_NO_THROW(m.deregisterLocalMemorySlot(s2));
}
