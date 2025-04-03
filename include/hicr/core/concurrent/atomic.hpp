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
 * @file concurrent/atomic.hpp
 * @brief Provides a generic wrapper for atomic datatypes
 * @author S. M. Martin
 * @date 19/11/2024
 */

#pragma once

#include <atomic>
#include <hicr/core/definitions.hpp>

namespace HiCR::concurrent
{

/**
 * @brief Generic class type for atomic datatypes
 *
 * @tparam T Represents the internal datatype
 */
template <class T>
class Atomic
{
  public:

  /**
   * Default constructor
  */
  Atomic()  = default;
  ~Atomic() = default;

  /**
 * Increments the value of the internal atomic object by one
 * 
 * @return The value of the object after the increment
 */
  __INLINE__ T increase() { return ((std::atomic<T> *)&_value)->fetch_add(1) + 1; }

  /**
 * Decrements the value of the internal atomic object by one
 * 
 * @return The value of the object after the decrement
 */
  __INLINE__ T decrease() { return ((std::atomic<T> *)&_value)->fetch_sub(1) - 1; }

  /**
 * Gets the current value of the internal atomic object
 * 
 * @return The value of the object
 */
  __INLINE__ T getValue() const { return ((std::atomic<T> *)&_value)->load(); }

  /**
 * Sets the current value of the internal atomic object, obtaining its previous value
 * 
 * @param value The value of the object to assign
 * @return Previous value before the update
 */
  [[nodiscard]] __INLINE__ T exchangeValue(const T value) const { return ((std::atomic<T> *)&_value)->exchange(value); }

  /**
 * Sets the current value of the internal atomic object
 * 
 * @param value The value of the object to assign
 */
  __INLINE__ void setValue(const T value) const { ((std::atomic<T> *)&_value)->store(value); }

  private:

  /**
   * Internal implementation of the concurrent queue
   */
  T _value;
};

} // namespace HiCR::concurrent
