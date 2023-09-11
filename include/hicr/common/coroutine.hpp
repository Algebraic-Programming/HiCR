/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file coroutine.hpp
 * @brief Provides a definition for the HiCR Coroutine class.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <boost/context/continuation.hpp>
#include <functional> // std::function
#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Defines the type accepted by the coroutine function as execution unit.
 *
 * \internal The question as to whether std::function entails too much overhead needs to evaluated, and perhaps deprecate it in favor of static function references. For the time being, this seems adequate enough.
 *
 */
typedef std::function<void(void *)> coroutineFc_t;

/**
 * Abstracts the basic functionality of a coroutine execution
 *
 * \internal Although it is currently tied to boost coroutines, the implementation needs to be abstracted further to support, for example, extensible stack coroutines (libmprompt)
 *
 */
class Coroutine
{
  public:

  Coroutine() = default;
  ~Coroutine() = default;

  /**
   * Resumes the execution of the coroutine. The coroutine needs to have been started before this, otherwise undefined behavior is to be expected.
   */
  __USED__ inline void resume()
  {
    _context = _context.resume();
  }

  /**
   * Yields the execution of the coroutine. The coroutine needs to be 'resumed' when running this function, otherwise undefined behavior is to be expected.
   */
  __USED__ inline void yield()
  {
    _context = _context.resume();
  }

  /**
   * Creates the context of the coroutine.
   *
   * This is separate from the class constructor to allow Just-in-time allocation of the stack. This enables the creation of many instances of this class, whereas only a few need to have an allocated stack at any given moment.
   *
   * \param[in] fc Function to run by the coroutine
   * \param[in] arg Argument to pass to the function
   */
  __USED__ inline void start(coroutineFc_t fc, void *arg)
  {
    const auto coroutineFc = [this, fc, arg](boost::context::continuation &&sink)
    {
      _context = std::move(sink);
      fc(arg);
      return std::move(_context);
    };

    // Creating new context
    _context = boost::context::callcc(coroutineFc);
  }

  private:

  /**
   * CPU execution context of the coroutine. This is the target context for the resume() function.
   */
  boost::context::continuation _context;
};

} // namespace HiCR
