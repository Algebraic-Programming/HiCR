/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrent/hashSet.hpp
 * @brief Provides generic support for parallel hash sets
 * @author S. M. Martin
 * @date 29/2/2023
 */

#pragma once

#include <parallel_hashmap/phmap.h>

namespace HiCR
{

namespace concurrent
{

/**
 * Template definition for a parallel hash set
 */
template <class V>
using HashSet = phmap::parallel_flat_hash_set<V, phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, std::allocator<V>, 4, std::mutex>;

} // namespace concurrent

} // namespace HiCR
