/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrentQueue.hpp
 * @brief Provides generic support for parallel hash sets
 * @author S. M. Martin
 * @date 29/2/2023
 */

#pragma once

#include <parallel_hashmap/phmap.h>

namespace HiCR
{

namespace common
{

/**
 * Template definition for a parallel hash set
 */
template <class V>
using parallelHashSet_t = phmap::parallel_flat_hash_set<V, phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, std::allocator<V>, 4, std::mutex>;

} // namespace common

} // namespace HiCR
