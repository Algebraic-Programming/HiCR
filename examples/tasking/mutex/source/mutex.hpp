#include <cstdio>
#include <cassert>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "../runtime.hpp"

#define _CONCURRENT_TASKS 1000ul

void mutex(HiCR::backend::host::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing runtime
  Runtime runtime;

  // Setting event handler on task sync to awaken the task that had been previously suspended on mutex
  runtime.setCallbackHandler(HiCR::tasking::Task::callback_t::onTaskSync, [&](HiCR::tasking::Task *task) { runtime.awakenTask(task); });

  // Assigning processing Re to TaskR
  for (const auto &computeResource : computeResources) runtime.addProcessingUnit(computeManager->createProcessingUnit(computeResource));

  // Contention value
  size_t value = 0;

  // Task-aware mutex
  HiCR::tasking::Mutex m;

  // Creating task function
  auto taskfc = computeManager->createExecutionUnit([&](void *arg) {
    // Getting pointer to current task
    auto task = (HiCR::tasking::Task *)arg;

    m.lock(task);
    value++;
    m.unlock(task);
  });

  // Running concurrent tasks
  for (size_t i = 0; i < _CONCURRENT_TASKS; i++) runtime.addTask(new Task(i, taskfc));

  // Running runtime
  runtime.run(computeManager);

  // Value should be equal to concurrent task count
  printf("Value %lu / Expected %lu\n", value, _CONCURRENT_TASKS);
  assert(value == _CONCURRENT_TASKS);
}
