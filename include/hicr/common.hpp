#pragma once

#include <../../extern/atomic_queue/atomic_queue.h>

namespace HiCR
{

// Maximum simultaneous tasks allowed
#ifndef MAX_SIMULTANEOS_TASKS
#define MAX_SIMULTANEOUS_TASKS 65536
#endif

// Size of the stack dedicated to the execution of tasks (coroutines)
#ifndef COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE 65536
#endif

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

// Lockfree queue
template <class T, unsigned int N> using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

} // namespace HiCR

