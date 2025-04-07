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
 * @brief Provides a base definition for a HiCR ExecutionUnit class
 * @author S. M. Martin
 * @date 06/10/2023
 */
#pragma once

#include <string>

namespace HiCR
{

/**
 * This class represents an abstract definition for a Execution Unit that:
 *
 * - Represents a single state-less computational operation (e.g., a function or a kernel) that can be replicated many times
 * - Is executed in the context of an ExecutionState class
 */
class ExecutionUnit
{
  public:

  /**
   * Indicates what type of execution unit is contained in this instance
   *
   * \return A string containing a human-readable description of the execution unit type
   */
  [[nodiscard]] virtual std::string getType() const = 0;

  /**
   * Default destructor
   */
  virtual ~ExecutionUnit() = default;

  protected:

  ExecutionUnit() = default;
};

} // namespace HiCR
