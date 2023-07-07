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

 // Task count
 std::atomic<uint64_t> _taskCount;

 // Matp between threads and workers
 std::map<pthread_t, Worker*> _workerIdToWorkerMap;

 // Lock-free queue for ready tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _readyTaskQueue;

 // Lock-free queue for waiting tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _waitingTaskQueue;

 // Hash map for quick location of tasks based on their hashed names
 HashSetT<taskLabel_t> _finishedTaskHashMap;
}; // class Runtime

} // namespace taskr


