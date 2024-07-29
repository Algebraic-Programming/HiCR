/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief This file implements variables shared among mutiple code files
 * @author Sergio Martin
 * @date 3/4/2024
 */

#pragma once

namespace HiCR
{

namespace tasking
{

/**
 * Flag to indicate whether the tasking subsystem was initialized
 */
extern bool _isInitialized;

/**
* Type definition for a number that can be used as universally unique identifier
*/
typedef uint64_t uniqueId_t;

/**
 * Definition for an callback callback. It includes a reference to the finished task
 */
template <class T>
using callbackFc_t = std::function<void(T)>;

} // namespace tasking

} // namespace HiCR
