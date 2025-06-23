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
 * @file instanceManager.cpp
 * @brief Unit tests for the hwloc instance manager backend
 * @author S. M. Martin
 * @date 16/8/2024
 */

#include <limits>
#include "gtest/gtest.h"
#include <hicr/core/instance.hpp>
#include <hicr/backends/hwloc/instanceManager.hpp>

TEST(MemoryManager, InstanceManager)
{
  HiCR::backend::hwloc::InstanceManager *im = NULL;

  EXPECT_NO_THROW(im = new HiCR::backend::hwloc::InstanceManager());
  EXPECT_FALSE(im == nullptr);
  delete im;
}

#define TEST_VALUE 42
TEST(MemoryManager, Lifetime)
{
  auto im = HiCR::backend::hwloc::InstanceManager::createDefault(nullptr, nullptr);

  EXPECT_ANY_THROW(im->addInstance(1));
  HiCR::Topology                          topology;
  std::shared_ptr<HiCR::InstanceTemplate> instanceTemplate(NULL);
  EXPECT_NO_THROW(instanceTemplate = im->createInstanceTemplate(topology));
  EXPECT_ANY_THROW(im->createInstance(*instanceTemplate));

  EXPECT_DEATH(im->abort(1), "");
}
