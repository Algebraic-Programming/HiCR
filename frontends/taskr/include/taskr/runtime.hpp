#pragma once
#include <atomic>
#include <hicr.hpp>
#include <map>
#include <taskr/common.hpp>

namespace taskr
{

class Task;
class Runtime;

// Static pointer to runtime singleton
inline Runtime *_runtime;

// Main Runtime
class Runtime
{
  public:
  // Instance of the HiSilicon Common Runtime
  HiCR::Runtime _hicr;

  // HiCR elements
  HiCR::EventMap *_eventMap;
  HiCR::Dispatcher *_dispatcher;
  std::vector<HiCR::Worker *> _workers;

  // Task count
  std::atomic<uint64_t> _taskCount;

  // Lock-free queue for waiting tasks
  lockFreeQueue_t<Task *, MAX_SIMULTANEOUS_TASKS> _waitingTaskQueue;

  // Hash map for quick location of tasks based on their hashed names
  HashSetT<taskLabel_t> _finishedTaskHashMap;
}; // class Runtime

} // namespace taskr
