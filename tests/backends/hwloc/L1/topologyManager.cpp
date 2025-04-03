/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

TEST(MemoryManager, Construction)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  HiCR::backend::hwloc::L1::TopologyManager *tm = NULL;

  EXPECT_NO_THROW(tm = new HiCR::backend::hwloc::L1::TopologyManager(&topology));
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
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

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
