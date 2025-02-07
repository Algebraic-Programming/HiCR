#include <cstdio>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include "../runtime.hpp"

#define ITERATIONS 10

void abcTasks(HiCR::backend::pthreads::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing runtime
  Runtime runtime;

  // Assigning processing Re to TaskR
  for (const auto &computeResource : computeResources) runtime.addProcessingUnit(computeManager->createProcessingUnit(computeResource));

  // Creating task functions
  auto taskAfc = computeManager->createExecutionUnit([&runtime](void *arg) { printf("Task A %lu\n", ((Task *)arg)->getLabel()); });
  auto taskBfc = computeManager->createExecutionUnit([&runtime](void *arg) { printf("Task B %lu\n", ((Task *)arg)->getLabel()); });
  auto taskCfc = computeManager->createExecutionUnit([&runtime](void *arg) { printf("Task C %lu\n", ((Task *)arg)->getLabel()); });

  // Now creating tasks and their dependency graph
  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto cTask = new Task(i * 3 + 2, taskCfc);
    cTask->addTaskDependency(i * 3 + 1);
    runtime.addTask(cTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto bTask = new Task(i * 3 + 1, taskBfc);
    bTask->addTaskDependency(i * 3 + 0);
    runtime.addTask(bTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto aTask = new Task(i * 3 + 0, taskAfc);
    if (i > 0) aTask->addTaskDependency(i * 3 - 1);
    runtime.addTask(aTask);
  }

  // Running runtime
  runtime.run(computeManager);
}
