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

#include <cstdint>
#include <functional>

namespace HiCR::tasking
{

/**
* Type definition for a number that can be used as universally unique identifier
*/
using uniqueId_t = uint64_t;

/**
 * Definition for an generic callback
 */
template <class T>
using callbackFc_t = std::function<void(T)>;

} // namespace HiCR::tasking
