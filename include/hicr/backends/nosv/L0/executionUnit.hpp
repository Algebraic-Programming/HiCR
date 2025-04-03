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
 * @brief nOS-V execution unit class. Main job is to store the function call
 * @author N. Baumann
 * @date 24/02/2025
 */
#pragma once

#include <hicr/backends/pthreads/L0/executionUnit.hpp>

namespace HiCR::backend::nosv::L0
{

/**
 * This class represents a replicable C++ executable function for the CPU-based backends
 */
class ExecutionUnit : public HiCR::L0::ExecutionUnit
{
  public:

  /**
   * Accepting a replicable C++ function type with closure parameter
   */
  using pthreadFc_t = std::function<void(void *)>;

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ExecutionUnit(pthreadFc_t fc)
    : HiCR::L0::ExecutionUnit(),
      _fc(std::move(fc)){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() override = default;

  [[nodiscard]] __INLINE__ std::string getType() const override { return "C++ Function"; }

  /**
   * Returns the internal function stored inside this execution unit
   *
   * \return The internal function stored inside this execution unit
   */
  [[nodiscard]] __INLINE__ pthreadFc_t &getFunction() { return _fc; }

  private:

  /**
   * Replicable internal C++ function to run in this execution unit
   */
  pthreadFc_t _fc;
};

} // namespace HiCR::backend::nosv::L0
