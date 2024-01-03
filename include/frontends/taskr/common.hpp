/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief This file implements common definitions required by TaskR
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <functional>
#include <frontends/taskr/concurrentQueue.hpp>
#include <hicr/common/definitions.hpp>

namespace taskr
{

/**
 * Task label type
 */
typedef uint64_t taskLabel_t;

#ifndef MAX_SIMULTANEOUS_WORKERS
  /**
   * Maximum simultaneous workers supported
   */
  #define MAX_SIMULTANEOUS_WORKERS 1024
#endif

#ifndef MAX_SIMULTANEOUS_TASKS
  /**
   * Maximum simultaneous tasks supported
   */
  #define MAX_SIMULTANEOUS_TASKS 65536
#endif

/**
 * Task callback function definition
 */
typedef std::function<void()> callback_t;

} // namespace taskr
