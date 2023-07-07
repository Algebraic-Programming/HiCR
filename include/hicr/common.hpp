#pragma once

#include <functional> // std::function

namespace HiCR
{

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

// Definition for task unique identifiers
typedef uint64_t taskId_t;

// Definition for a task function - supports lambda functions
typedef std::function<void(void*)> taskFunction_t;

// Definition for an event callback. It includes referenced task id.
typedef std::function<void(taskId_t taskId)> eventCallback_t;

} // namespace HiCR

