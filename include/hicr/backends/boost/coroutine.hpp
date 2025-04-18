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
 * @file coroutine.hpp
 * @brief Provides a definition for the Coroutine class, implemented with Boost context objects.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <functional>
#include <boost/context/continuation.hpp>
#include <hicr/core/exceptions.hpp>

namespace HiCR::backend::boost
{

/**
 * Abstracts the basic functionality of a coroutine execution
 *
 * \internal Although it is currently tied to boost coroutines, the implementation needs to be abstracted further to support, for example, extensible stack coroutines (libmprompt)
 *
 */
class Coroutine
{
  public:

  /**
   * Defines the type accepted by the coroutine function
   *
   * \internal The question as to whether std::function entails too much overhead needs to evaluated, and perhaps deprecate it in favor of static function references. For the time being, this seems adequate enough.
   *
   */
  using coroutineFc_t = std::function<void(void *)>;

  /**
   * Resumes the execution of the coroutine. The coroutine needs to have been started before this, otherwise undefined behavior is to be expected.
   */
  __INLINE__ void resume()
  {
    if (_hasFinished == true) HICR_THROW_RUNTIME("Attempting to resume a coroutine that has already finished");
    if (_runningContext == true) HICR_THROW_RUNTIME("Attempting to resume a coroutine that is already running");

    // Setting coroutine to have entered the running context
    _runningContext = true;

    // Resuming
    _context = _context.resume();
  }

  /**
   * Yields the execution of the coroutine. The coroutine needs to be 'resumed' when running this function, otherwise undefined behavior is to be expected.
   */
  __INLINE__ void yield()
  {
    if (_hasFinished == true) HICR_THROW_RUNTIME("Attempting to suspend a coroutine that has already finished");
    if (_runningContext == false) HICR_THROW_RUNTIME("Attempting to suspend a coroutine that is not running");

    // Setting coroutine to have exited the running context
    _runningContext = false;

    // Yielding
    _context = _context.resume();
  }

  /**
   * Creates the context of the coroutine.
   *
   * This is separate from the class constructor to allow Just-in-time allocation of the stack. This enables the creation of many instances of this class, whereas only a few need to have an allocated stack at any given moment.
   *
   * \param[in] fc Function to run by the coroutine
   * \param[in] arg Argument (closure) to be passed to the function
   */
  __INLINE__ void start(const coroutineFc_t &fc, void *const arg)
  {
    const auto coroutineFc = [this, fc, arg](::boost::context::continuation &&sink) {
      // Storing caller context
      _context = std::move(sink);

      // First yield allows the creation of a coroutine without running the function
      yield();

      // Executing coroutine function
      fc(arg);

      // Setting the coroutine as finished
      _hasFinished = true;

      // Setting coroutine to have exited the running context
      _runningContext = false;

      // Returning to caller context
      return std::move(_context);
    };

    // Setting coroutine to have entered the running context
    _runningContext = true;

    // Creating new context
    _context = ::boost::context::callcc(coroutineFc);
  }

  /**
   * A function to check whether the coroutine has finished execution completely
   *
   * \return True, if the coroutine has finished; False, otherwise.
   */
  __INLINE__ bool hasFinished() { return _hasFinished; }

  private:

  /**
   * This variable serves as safeguard in case the function has finished but the user wants to resume it
   */
  bool _hasFinished = false;

  /**
   * This variable serves as safeguard to prevent double resume/yield
   */
  bool _runningContext = false;

  /**
   * CPU execution context of the coroutine. This is the target context for the resume() function.
   */
  ::boost::context::continuation _context;
};

} // namespace HiCR::backend::boost
