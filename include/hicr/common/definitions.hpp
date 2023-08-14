#pragma once

#include <../../extern/atomic_queue/atomic_queue.h>

namespace HiCR
{

// Maximum simultaneous tasks allowed in a task queue
#ifndef MAX_QUEUED_TASKS
  #define MAX_QUEUED_TASKS 65536
#endif

// Size of the stack dedicated to the execution of tasks (coroutines)
#ifndef COROUTINE_STACK_SIZE
  #define COROUTINE_STACK_SIZE 65536
#endif

} // namespace HiCR
