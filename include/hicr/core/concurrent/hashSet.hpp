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
 * @file concurrent/hashSet.hpp
 * @brief Provides generic support for parallel hash sets
 * @author S. M. Martin
 * @date 29/2/2023
 */

#pragma once

#include <parallel_hashmap/phmap.h>

namespace HiCR::concurrent
{

/**
 * Template definition for a parallel hash set
 */
template <class V>
using HashSet = phmap::parallel_flat_hash_set<V, phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, std::allocator<V>, 4, std::mutex>;

} // namespace HiCR::concurrent
