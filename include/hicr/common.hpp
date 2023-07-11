#pragma once

#include <functional> // std::function
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

// Definition for task unique identifiers
typedef uint64_t taskId_t;

// Definition for a task function - supports lambda functions
typedef std::function<void(void*)> taskFunction_t;

// Definition for an event callback. It includes referenced task id.
typedef std::function<void(taskId_t taskId)> eventCallback_t;

// Lockfree queue
template <class T, unsigned int N> using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

} // namespace HiCR

