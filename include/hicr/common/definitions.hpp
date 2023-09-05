#pragma once

#include <hicr/common/atomic_queue/atomic_queue.h>
#include <hicr/common/parallel_hashmap/phmap.h>

namespace HiCR
{

// API annotation for inline functions, to make sure they are considered during coverage analysis
// This solution was taken from https://stackoverflow.com/a/10824832
#if defined __GNUC__ && defined ENABLE_COVERAGE
  #define __USED__ __attribute__((__used__))
#else
  #define __USED__
#endif

/**
 * Templated Lockfree queue definition
 */
template <class T, unsigned int N>
using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

/**
 * Template definition for parallel hash set
 */
template <class V>
using parallelHashSet_t = phmap::parallel_flat_hash_set<V, phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, std::allocator<V>, 4, std::mutex>;

// Configuration for parallel hash maps
template <class K, class V>
using parallelHashMap_t = phmap::parallel_flat_hash_map<K, V, phmap::priv::hash_default_hash<K>, phmap::priv::hash_default_eq<K>, std::allocator<std::pair<const K, V>>, 4, std::mutex>;

} // namespace HiCR
