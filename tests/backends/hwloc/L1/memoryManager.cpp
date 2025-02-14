/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.cpp
 * @brief Unit tests for the HWLoc-based memory manager backend
 * @author S. M. Martin
 * @date 13/9/2023
 */

#include <limits>
#include "gtest/gtest.h"
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/hwloc/L1/memoryManager.hpp>
#include <hicr/backends/pthreads/L1/communicationManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

TEST(MemoryManager, Construction)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  HiCR::backend::hwloc::L1::MemoryManager *m = NULL;

  EXPECT_NO_THROW(m = new HiCR::backend::hwloc::L1::MemoryManager(&topology));
  EXPECT_FALSE(m == nullptr);
  delete m;
}

TEST(MemoryManager, Memory)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);
  HiCR::backend::hwloc::L1::MemoryManager           m(&topology);
  HiCR::backend::pthreads::L1::CommunicationManager c;

  // Initializing hwloc-based topology manager
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Asking hwloc to check the available devices
  HiCR::L0::Topology t;
  EXPECT_NO_THROW(t = tm.queryTopology());

  // Getting first device found
  HiCR::L0::Topology::deviceList_t dList;
  EXPECT_NO_THROW(dList = t.getDevices());
  ASSERT_GT(dList.size(), (unsigned int)0);
  auto d = *dList.begin();

  // Getting memory resource list (should be size 1)
  HiCR::L0::Device::memorySpaceList_t mList;
  EXPECT_NO_THROW(mList = d->getMemorySpaceList());
  ASSERT_GT(mList.size(), (unsigned int)0);

  // Getting memory resource
  auto           r = *mList.begin();
  nlohmann::json serializedMemSlot;
  EXPECT_NO_THROW(serializedMemSlot = r->serialize());
  EXPECT_NO_THROW(r->deserialize(serializedMemSlot));

  // Getting total memory size
  size_t testMemAllocSize = 1024;
  size_t totalMem         = 0;
  EXPECT_NO_THROW(totalMem = r->getSize());

  // Making sure the system has enough memory for the next test
  ASSERT_GE(totalMem, testMemAllocSize);

  // Trying to allocate more than allowed
  EXPECT_THROW(m.allocateLocalMemorySlot(r, std::numeric_limits<ssize_t>::max()), HiCR::LogicException);

  // Allocating memory correctly now
  std::shared_ptr<HiCR::L0::LocalMemorySlot> s1 = NULL;
  EXPECT_NO_THROW(s1 = m.allocateLocalMemorySlot(r, testMemAllocSize));
  EXPECT_EQ(s1->getSize(), testMemAllocSize);

  // Getting local pointer from allocation
  void *s1LocalPtr = NULL;
  EXPECT_NO_THROW(s1LocalPtr = s1->getPointer());
  memset(s1LocalPtr, 0, testMemAllocSize);

  // Creating memory slot from a previous allocation
  std::shared_ptr<HiCR::L0::LocalMemorySlot> s2 = NULL;
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
  EXPECT_NO_THROW(c.memcpy(s2, 0, s1, 0, testMessage.size()));

  // Force memcpy operation to finish
  EXPECT_NO_THROW(c.fence(0));

  // Making sure the message was received
  bool sameStrings = true;
  for (size_t i = 0; i < testMemAllocSize; i++)
    if (((const char *)s1LocalPtr)[i] != ((const char *)s2LocalPtr)[i]) sameStrings = false;
  EXPECT_TRUE(sameStrings);

  // Freeing memory slots
  EXPECT_NO_THROW(m.freeLocalMemorySlot(s1));
  EXPECT_NO_THROW(m.deregisterLocalMemorySlot(s2));
}

// Test the MemoryManager::memset operation
TEST(MemoryManager, Memset)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);
  HiCR::backend::hwloc::L1::MemoryManager m(&topology);

  // Initializing hwloc-based topology manager
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Asking hwloc to check the available devices
  HiCR::L0::Topology t;
  EXPECT_NO_THROW(t = tm.queryTopology());

  // Getting first device found
  HiCR::L0::Topology::deviceList_t dList;
  EXPECT_NO_THROW(dList = t.getDevices());
  ASSERT_GT(dList.size(), (unsigned int)0);
  auto d = *dList.begin();

  // Getting memory resource list (should be size 1)
  HiCR::L0::Device::memorySpaceList_t mList;
  EXPECT_NO_THROW(mList = d->getMemorySpaceList());
  ASSERT_GT(mList.size(), (unsigned int)0);

  // Getting memory resource
  auto r = *mList.begin();

  // Getting total memory size
  size_t testMemAllocSize = 1024;
  size_t totalMem         = 0;
  EXPECT_NO_THROW(totalMem = r->getSize());

  // Making sure the system has enough memory for the next test
  ASSERT_GE(totalMem, testMemAllocSize);

  // Allocating memory correctly now
  std::shared_ptr<HiCR::L0::LocalMemorySlot> s1 = NULL;
  EXPECT_NO_THROW(s1 = m.allocateLocalMemorySlot(r, testMemAllocSize));
  EXPECT_EQ(s1->getSize(), testMemAllocSize);

  // Getting local pointer from allocation
  uint8_t *s1LocalPtr = NULL;
  EXPECT_NO_THROW(s1LocalPtr = (uint8_t *)s1->getPointer());

  // Filling memory slot with value 0
  EXPECT_NO_THROW(m.memset(s1, 0));
  EXPECT_EQ(s1LocalPtr[0], 0);
  EXPECT_EQ(s1LocalPtr[testMemAllocSize - 1], 0);

  // Filling half the memory slot with value 9
  EXPECT_NO_THROW(m.memset(s1, 9, testMemAllocSize / 2));
  EXPECT_EQ(s1LocalPtr[0], 9);
  EXPECT_EQ(s1LocalPtr[testMemAllocSize / 2 - 1], 9);
  EXPECT_EQ(s1LocalPtr[testMemAllocSize / 2 + 1], 0);
  EXPECT_EQ(s1LocalPtr[testMemAllocSize - 1], 0);

  // Freeing memory slots
  EXPECT_NO_THROW(m.freeLocalMemorySlot(s1));
}
