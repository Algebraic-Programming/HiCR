#include <cstdio>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include "../runtime.hpp"

#define ITERATIONS 10

void abcTasks(Runtime &runtime)
{
  // Creating task functions
  auto taskAfc = [&runtime](void *arg) { printf("Task A %lu\n", ((Task *)arg)->getLabel()); };
  auto taskBfc = [&runtime](void *arg) { printf("Task B %lu\n", ((Task *)arg)->getLabel()); };
  auto taskCfc = [&runtime](void *arg) { printf("Task C %lu\n", ((Task *)arg)->getLabel()); };

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
  runtime.run();
}
