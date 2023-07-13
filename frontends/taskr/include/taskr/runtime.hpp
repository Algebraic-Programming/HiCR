#pragma once
#include <map>
#include <atomic>
#include <hicr.hpp>
#include <hicr/logger.hpp>
#include <taskr/common.hpp>

namespace taskr
{

class Task;
class Worker;
class Runtime;

// Static pointer to runtime singleton
inline Runtime* _runtime;

// Main Runtime
class Runtime
{
public:

 // Instance of the HiSilicon Common Runtime
 HiCR::Runtime _hicr;

 // HiCR elements
 HiCR::EventMap eventMap;
 std::vector<HiCR::Backend*> backends;
 std::vector<HiCR::Resource*> resources;
 std::vector<HiCR::Dispatcher*> dispatchers;
 std::vector<HiCR::Task*> workers;
 std::map<HiCR::Task*, HiCR::Resource*> taskToResourceMap;

 // Worker count
 std::atomic<uint64_t> _workerCount;

 // Task count
 std::atomic<uint64_t> _taskCount;

 // Lock-free queue for ready tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _readyTaskQueue;

 // Lock-free queue for waiting tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _waitingTaskQueue;

 // Hash map for quick location of tasks based on their hashed names
 HashSetT<taskLabel_t> _finishedTaskHashMap;
}; // class Runtime

} // namespace taskr


