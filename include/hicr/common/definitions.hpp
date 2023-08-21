#pragma once

#include <../../extern/atomic_queue/atomic_queue.h>

namespace HiCR
{

// Size of the stack dedicated to the execution of tasks (coroutines)
#ifndef COROUTINE_STACK_SIZE
  #define COROUTINE_STACK_SIZE 65536
#endif

} // namespace HiCR
