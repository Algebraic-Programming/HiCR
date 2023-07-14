#pragma once

#include <taskr/extern/parallel_hashmap/phmap.h>
#include <taskr/extern/atomic_queue/atomic_queue.h>
#include <functional>

namespace taskr
{

// Task label type
typedef uint64_t taskLabel_t;

// Hash type
typedef uint64_t hash_t;

// Maximum simultaneous tasks allowed
#ifndef MAX_SIMULTANEOS_TASKS
#define MAX_SIMULTANEOUS_TASKS 65536
#endif

// Size of the stack dedicated to the execution of tasks (coroutines)
#ifndef COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE 65536
#endif

// Lockfree queues
template <class T, unsigned int N> using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

// Callback type
typedef std::function<void()> callback_t;

// Configuration for parallel hash sets
#define SETNAME phmap::parallel_flat_hash_set
#define SETEXTRAARGS , phmap::priv::hash_default_hash<V>, phmap::priv::hash_default_eq<V>, std::allocator<V>, 4, std::mutex
template <class V> using HashSetT = SETNAME<V SETEXTRAARGS>;


} // namespace taskr


