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
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the Boost backend
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/executionUnit.hpp>
#include <utility>
#include "coroutine.hpp"

namespace HiCR::backend::boost
{

/**
 * This class represents a replicable C++ executable function for the CPU-based backends
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ExecutionUnit(Coroutine::coroutineFc_t fc)
    : HiCR::ExecutionUnit(),
      _fc(std::move(fc)){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() override = default;

  [[nodiscard]] __INLINE__ std::string getType() const override { return "Boost C++ Coroutine Function"; }

  /**
   * Returns the internal function stored inside this execution unit
   *
   * \return The internal function stored inside this execution unit
   */
  [[nodiscard]] __INLINE__ const Coroutine::coroutineFc_t &getFunction() const { return _fc; }

  private:

  /**
   * Replicable internal C++ function to run in this execution unit
   */
  const Coroutine::coroutineFc_t _fc;
};

} // namespace HiCR::backend::boost
