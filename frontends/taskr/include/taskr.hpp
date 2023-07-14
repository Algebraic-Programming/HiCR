#pragma once
#include <taskr/runtime.hpp>
#include <taskr/task.hpp>
#include <taskr/worker.hpp>
#include <omp.h>

namespace taskr
{

// Boolean indicating whether the runtime system was initialized
inline bool _runtimeInitialized = false;

inline void addTask(Task* task)
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Increasing task count
 _runtime->_taskCount++;

 // Checking if maximum was exceeded
 if (_runtime->_taskCount >= MAX_SIMULTANEOUS_TASKS) LOG_ERROR("Maximum task size (MAX_SIMULTANEOUS_TASKS = %lu) exceeded.\n", MAX_SIMULTANEOUS_TASKS);

 // Adding task to the waiting lis, it will be cleared out later
 _runtime->_waitingTaskQueue.push(task);
}

inline void initialize()
{
 // Instantiating singleton
 _runtime = new Runtime();

 // Initializing HiCR runtime
 _runtime->_hicr.initialize();

 // Set initialized state
 _runtimeInitialized = true;
}


// This function finds a task in the waiting queue and checks its dependencies
static inline HiCR::Task* checkWaitingTasks()
{
 // Pointer to the next task to execute
 Task* task;

 // Poping next task from the lock-free queue
 bool foundTask = _runtime->_waitingTaskQueue.try_pop(task);

 // If no task was found (queue was empty), then just an empty task
 if (foundTask == false) return NULL;

 // Check if task is ready now
 bool isTaskReady = task->isReady();

 // If it is, put it in the ready-to-go queue
 if (isTaskReady == true)
 {
  // If a task was found (queue was not empty), then execute and manage the task depending on its state
  auto hicrTask = new HiCR::Task();
  hicrTask->setFunction([task](void* arg){task->run();});
  hicrTask->setEventMap(_runtime->_eventMap);
  hicrTask->setArgument(task);
  return hicrTask;
 }

 // Otherwise, put it at the back of the waiting task pile and return
 _runtime->_waitingTaskQueue.push(task);

 // And return a null task
 return NULL;
}

static inline void onTaskFinish(HiCR::Task* hicrTask)
{
  // Getting TaskR task
  auto task = (Task*) hicrTask->getArgument();

  // Decreasing overall task count
  _runtime->_taskCount--;

  // If all tasks finished, then terminate execution
  if (_runtime->_taskCount == 0) for (auto r : _runtime->_resources) r->finalize();

  // Adding task label to finished task set
  _runtime->_finishedTaskHashMap.insert(task->getLabel());

  // Free task's memory to prevent leaks. Could not use unique_ptr because the type is not supported by boost's lock-free queue
  delete task;
}

inline void run()
{
 if (_runtimeInitialized == false) LOG_ERROR("Attempting to use Taskr without first initializing it.");

 // Querying HiCR for available workers
 _runtime->_backends = _runtime->_hicr.getBackends();
 _runtime->_dispatcher = new HiCR::Dispatcher();
 _runtime->_eventMap = new HiCR::EventMap();

 // Setting task pull function for dispatcher
 _runtime->_dispatcher->setPullFunction(&checkWaitingTasks);

 // Creating event map ands events
  _runtime->_eventMap->setEvent(HiCR::event_t::onTaskFinish, [](HiCR::Task* task){onTaskFinish(task);});

 // Gathering all resources that can execute worker tasks
 for (size_t i = 0; i < _runtime->_backends.size(); i++)
 {
  _runtime->_backends[i]->queryResources();
  const auto& backendResources = _runtime->_backends[i]->getResourceList();
  _runtime->_resources.insert(_runtime->_resources.end(), backendResources.begin(), backendResources.end());
 }

 // Subscribing resources to the main dispatcher
 for (size_t i = 0; i < _runtime->_resources.size(); i++)_runtime->_resources[i]->subscribe(_runtime->_dispatcher);

 // Initializing workers
 for (size_t i = 0; i < _runtime->_resources.size(); i++) _runtime->_resources[i]->initialize();

 // Waiting for workers to finish (i.e., all tasks have finished)
 for (size_t i = 0; i < _runtime->_resources.size(); i++) _runtime->_resources[i]->await();

 // Clearing resources
 for (auto& x : _runtime->_backends) delete x;
 for (auto& x : _runtime->_resources) delete x;
 _runtime->_backends.clear();
 _runtime->_resources.clear();
 delete _runtime->_dispatcher;
 delete _runtime->_eventMap;
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

