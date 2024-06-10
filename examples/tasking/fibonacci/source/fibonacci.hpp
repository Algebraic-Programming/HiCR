#include <cstdio>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "../runtime.hpp"

static HiCR::backend::host::L1::ComputeManager *_computeManager;
static taskr::Runtime                          *_taskr;
static std::atomic<uint64_t>                    _taskCounter;

// This function serves to encode a fibonacci task label
inline const uint64_t getFibonacciLabel(const uint64_t x, const uint64_t _initialValue) { return _initialValue; }

// Fibonacci without memoization to stress the tasking runtime
uint64_t fibonacci(const uint64_t x)
{
  if (x == 0) return 0;
  if (x == 1) return 1;

  uint64_t result1 = 0;
  uint64_t result2 = 0;
  auto     fibFc1  = _computeManager->createExecutionUnit([&]() { result1 = fibonacci(x - 1); });
  auto     fibFc2  = _computeManager->createExecutionUnit([&]() { result2 = fibonacci(x - 2); });

  uint64_t taskId1 = _taskCounter.fetch_add(1);
  uint64_t taskId2 = _taskCounter.fetch_add(1);

  auto task1 = new HiCR::tasking::Task(taskId1, fibFc1);
  auto task2 = new HiCR::tasking::Task(taskId2, fibFc2);

  _taskr->addTask(task1);
  _taskr->addTask(task2);

  _taskr->getCurrentTask()->addTaskDependency(taskId1);
  _taskr->getCurrentTask()->addTaskDependency(taskId2);

  _taskr->getCurrentTask()->suspend();

  return result1 + result2;
}

uint64_t fibonacciDriver(const uint64_t initialValue, HiCR::backend::host::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing taskr
  taskr::Runtime taskr;

  // Setting event handler to re-add task to the queue after it suspended itself
  taskr.setEventHandler(HiCR::tasking::Task::event_t::onTaskSuspend, [&](HiCR::tasking::Task *task) { taskr.awakenTask(task); });

  // Setting global variables
  _taskr          = &taskr;
  _computeManager = computeManager;
  _taskCounter    = 0;

  // Assigning processing resource to TaskR
  for (const auto &computeResource : computeResources) taskr.addProcessingUnit(computeManager->createProcessingUnit(computeResource));

  // Storage for result
  uint64_t result = 0;

  // Creating task functions
  auto initialFc = computeManager->createExecutionUnit([&]() { result = fibonacci(initialValue); });

  // Now creating tasks and their dependency graph
  auto initialTask = new HiCR::tasking::Task(_taskCounter.fetch_add(1), initialFc);
  taskr.addTask(initialTask);

  // Running taskr
  taskr.run(computeManager);

  // Returning fibonacci value
  return result;
}
