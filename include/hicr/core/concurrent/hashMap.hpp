/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrent/hashMap.hpp
 * @brief Provides generic support for parallel hash maps
 * @author S. M. Martin
 * @date 29/2/2023
 */

#pragma once

#include <parallel_hashmap/phmap.h>

namespace HiCR::concurrent
{

/**
 * Template definition for a parallel hash set
 * 
 * @tparam T Represents the internal key datatype
 * @tparam V Represents the internal value datatype
 */
template <class K, class V>
using HashMap = phmap::parallel_flat_hash_map<K, V, phmap::priv::hash_default_hash<K>, phmap::priv::hash_default_eq<K>, std::allocator<std::pair<const K, V>>, 4, std::mutex>;

} // namespace HiCR::concurrent
