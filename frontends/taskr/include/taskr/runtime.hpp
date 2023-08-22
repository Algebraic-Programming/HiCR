#pragma once
#include <atomic>
#include <hicr.hpp>
#include <map>
#include <mutex>
#include <taskr/common.hpp>
#include <taskr/task.hpp>

namespace taskr
{

class Runtime;

// Static pointer to runtime singleton
inline Runtime *_runtime;

// Main Runtime
class Runtime
{
  private:
  // Instance of the HiSilicon Common Runtime
  HiCR::Runtime _hicr;

  // HiCR elements
  HiCR::EventMap<HiCR::Task> *_eventMap;
  HiCR::Dispatcher *_dispatcher;
  std::vector<HiCR::Worker *> _workers;

  // Task count
  std::atomic<uint64_t> _taskCount;

  // Lock-free queue for waiting tasks
  lockFreeQueue_t<Task *, MAX_SIMULTANEOUS_TASKS> _waitingTaskQueue;

  // Hash map for quick location of tasks based on their hashed names
  HashSetT<taskLabel_t> _finishedTaskHashMap;

  // Storage required for auto-sleeping workers
  std::mutex _activeWorkerQueueLock;
  ssize_t _maximumActiveWorkers = -1;
  ssize_t _activeWorkerCount;
  lockFreeQueue_t<HiCR::Worker *, MAX_SIMULTANEOUS_WORKERS> _suspendedWorkerQueue;

  inline bool checkTaskReady(Task *task)
  {
    const auto &dependencies = task->getDependencies();

    // Checking Task dependencies
    for (size_t i = 0; i < dependencies.size(); i++)
      if (_finishedTaskHashMap.contains(dependencies[i]) == false) return false; // If any unsatisfied dependency was found, return immediately

    return true;
  }

  inline void checkMaximumActiveWorkerCount(HiCR::Worker *worker)
  {
    // Try to get the active worker queue lock, otherwise keep going
    if (_activeWorkerQueueLock.try_lock())
    {
      // If the number of workers exceeds that of maximum active workers, suspend current worker
      if (_maximumActiveWorkers > 0 && _activeWorkerCount > _maximumActiveWorkers)
      {
        // Adding worker to the queue
        _suspendedWorkerQueue.push(worker);

        // Reducing active worker count
        _activeWorkerCount--;

        // Releasing lock
        _activeWorkerQueueLock.unlock();

        // Suspending worker
        worker->suspend();

        // Returning now
        return;
      }

      // If the new maximum is higher than the number of active workers, we need to re-awaken some of them
      while ((_maximumActiveWorkers < 0 || _maximumActiveWorkers > _activeWorkerCount) && _suspendedWorkerQueue.was_size() > 0)
      {
        // Pointer to the worker to wake up
        HiCR::Worker *w = NULL;

        // Getting the worker from the queue of suspended workers
        _suspendedWorkerQueue.try_pop(w);

        // Increase the active worker count
        _activeWorkerCount++;

        // Resuming worker
        w->resume();
      }

      // Releasing lock
      _activeWorkerQueueLock.unlock();
    }
  }

  public:
  inline void initialize()
  {
    // Initializing HiCR runtime
    _runtime->_hicr.initialize();
  }

  inline void setMaximumActiveWorkers(const ssize_t max)
  {
    // Storing new maximum active worker count
    _runtime->_maximumActiveWorkers = max;
  }

  inline void addTask(Task *task)
  {
    // Increasing task count
    _taskCount++;

    // Checking if maximum was exceeded
    if (_taskCount >= MAX_SIMULTANEOUS_TASKS) LOG_ERROR("Maximum task size (MAX_SIMULTANEOUS_TASKS = %lu) exceeded.\n", MAX_SIMULTANEOUS_TASKS);

    // Adding task to the waiting lis, it will be cleared out later
    _waitingTaskQueue.push(task);
  }

  inline void onTaskFinish(HiCR::Task *hicrTask)
  {
    // Getting TaskR task
    auto task = (Task *)hicrTask->getArgument();

    // Decreasing overall task count
    _taskCount--;

    // If all tasks finished, then terminate execution
    if (_taskCount == 0)
      for (auto w : _workers) w->terminate();

    // Adding task label to finished task set
    _finishedTaskHashMap.insert(task->getLabel());

    // Free task's memory to prevent leaks. Could not use unique_ptr because the type is not supported by boost's lock-free queue
    delete task;
  }

  // This function finds a task in the waiting queue and checks its dependencies
  inline HiCR::Task *checkWaitingTasks(HiCR::Worker *worker)
  {
    // Pointer to the next task to execute
    Task *task;

    // If maximum active workers is defined, then check if the threshold is exceeded
    checkMaximumActiveWorkerCount(worker);

    // Poping next task from the lock-free queue
    bool foundTask = _waitingTaskQueue.try_pop(task);

    // If no task was found (queue was empty), then return an empty task
    if (foundTask == false) return NULL;

    // Check if task is ready now
    bool isTaskReady = checkTaskReady(task);

    // If it is, put it in the ready-to-go queue
    if (isTaskReady == true)
    {
      // If a task was found (queue was not empty), then execute and manage the task depending on its state
      auto hicrTask = task->getHiCRTask();
      hicrTask->setEventMap(_eventMap);
      return hicrTask;
    }

    // Otherwise, put it at the back of the waiting task pile and return
    _waitingTaskQueue.push(task);

    // And return a null task
    return NULL;
  }

  inline void run()
  {
    _dispatcher = new HiCR::Dispatcher([this](HiCR::Worker *worker)
                                       { return checkWaitingTasks(worker); });
    _eventMap = new HiCR::EventMap<HiCR::Task>();

    // Creating event map ands events
    _eventMap->setEvent(HiCR::event_t::onTaskFinish, [this](HiCR::Task *task)
                        { onTaskFinish(task); });

    // Gathering all resources that can execute worker tasks
    for (size_t i = 0; i < _hicr.getBackends().size(); i++)
    {
      _hicr.getBackends()[i]->queryResources();

      // Creating one worker per thread
      for (auto &resource : _hicr.getBackends()[i]->getComputeResourceList())
      {
        auto worker = new HiCR::Worker();

        // Assigning resource to the thread
        worker->addResource(resource.get());

        // Assigning worker to the common dispatcher
        worker->subscribe(_dispatcher);

        // Finally adding worker to the worker set
        _workers.push_back(worker);

        // Initializing worker
        worker->initialize();
      }
    }

    // Initializing active worker count
    _activeWorkerCount = _workers.size();

    // Starting workers
    for (auto &w : _workers) w->start();

    // Waiting for workers to finish
    for (auto &w : _workers) w->await();

    // Finalizing resources
    for (auto &w : _workers)
      for (auto &r : w->getResources()) r->finalize();

    // Clearing created objects
    for (auto &w : _workers) delete w;
    _workers.clear();
    delete _dispatcher;
    delete _eventMap;
  }

}; // class Runtime

} // namespace taskr
