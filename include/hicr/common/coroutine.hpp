#pragma once

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file coroutine.hpp
 * @desc Provides basic abstractions for coroutine support.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#include <boost/context/continuation.hpp>
#include <functional> // std::function

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
  inline void resume()
  {
    _contextSource = _contextSource.resume();
  }

  /**
   * Yields the execution of the coroutine. The coroutine needs to be 'resumed' when running this function, otherwise undefined behavior is to be expected.
   */
  inline void yield()
  {
    _contextSink = _contextSink.resume();
  }

  /**
   * Creates the context of the coroutine.
   *
   * This is separate from the class constructor to allow Just-in-time allocation of the stack. This enables the creation of many instances of this class, whereas only a few need to have an allocated stack at any given moment.
   */
  inline void start(coroutineFc_t &fc, void *arg)
  {
    auto coroutineFc = [this, fc, arg](boost::context::continuation &&sink)
    {
      _contextSink = std::move(sink);
      fc(arg);
      yield();
      return std::move(sink);
    };

    // Creating new context
    _contextSource = boost::context::callcc(coroutineFc);
  }

  private:

  /**
   * CPU execution context of the coroutine. This is the target context for the resume() function.
   */
  boost::context::continuation _contextSource;

  /**
   * CPU execution context of the resume() caller. This is the return context for the yield() function.
   */
  boost::context::continuation _contextSink;
};

} // namespace HiCR
