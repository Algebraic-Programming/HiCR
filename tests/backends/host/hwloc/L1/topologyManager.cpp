/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.cpp
 * @brief Unit tests for the HWLoc-based memory manager backend
 * @author S. M. Martin
 * @date 16/8/2024
 */

#include <limits>
#include "gtest/gtest.h"
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

TEST(MemoryManager, Construction)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  HiCR::backend::host::hwloc::L1::TopologyManager *tm = NULL;

  EXPECT_NO_THROW(tm = new HiCR::backend::host::hwloc::L1::TopologyManager(&topology));
  EXPECT_FALSE(tm == nullptr);
  delete tm;
}

TEST(MemoryManager, Memory)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing hwloc-based topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking hwloc to check the available devices
  HiCR::L0::Topology t;
  EXPECT_NO_THROW(t = tm.queryTopology());

  // Serializing topology
  nlohmann::json serializedTopology;
  EXPECT_NO_THROW(serializedTopology = t.serialize());
  EXPECT_NO_THROW(tm.deserializeTopology(serializedTopology));

  nlohmann::json serializedTopology2;
  EXPECT_NO_THROW(serializedTopology2 = t.serialize());
  EXPECT_EQ(serializedTopology, serializedTopology2);
}
