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
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 4/4/2024
 */
#pragma once

#include <hicr/core/instance.hpp>

namespace HiCR::backend::hwloc
{

/**
 * This class represents an abstract definition for a HICR instance as represented by the hwloc backend
 */
class Instance final : public HiCR::Instance
{
  public:

  /**
   * Constructor for a Instance class for the hwloc backend
   */
  Instance()
    : HiCR::Instance(0)
  {}

  /**
   * Default destructor
   */
  ~Instance() override = default;

  [[nodiscard]] bool isRootInstance() const override
  {
    // Only a single instance exists (the currently running one), hence always true
    return true;
  };
};

} // namespace HiCR::backend::hwloc
