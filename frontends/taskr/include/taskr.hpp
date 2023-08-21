#pragma once
#include <taskr/runtime.hpp>
#include <taskr/task.hpp>

namespace taskr
{

// Boolean indicating whether the runtime system was initialized
inline bool _runtimeInitialized = false;

inline void addTask(Task *task)
{
  if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Adding task
  _runtime->addTask(task);
}

inline void initialize()
{
  // Instantiating singleton
  _runtime = new Runtime();

  // Initializing Taskr
  _runtime->initialize();

  // Set initialized state
  _runtimeInitialized = true;
}

inline void setMaximumActiveWorkers(const ssize_t max)
{
  // Storing new maximum active worker count
  _runtime->setMaximumActiveWorkers(max);
}

inline void run()
{
  if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Running Taskr
  _runtime->run();
}

inline void finalize()
{
  if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Freeing up singleton
  delete _runtime;

  // Setting runtime to non-initialized
  _runtimeInitialized = false;
}

} // namespace taskr
