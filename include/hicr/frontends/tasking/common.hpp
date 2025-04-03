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
