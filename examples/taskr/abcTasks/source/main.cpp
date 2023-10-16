#include <cstdio>
#include <cstring>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>

#define ITERATIONS 100

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Pthreads backend to run in parallel
  HiCR::backend::sharedMemory::ComputeManager computeManager(&topology);

  // Querying computational resources
  computeManager.queryComputeResources();

  // Updating the compute resource list
  auto computeResources = computeManager.getComputeResourceList();

  // Initializing taskr
  auto _taskr = new taskr::Runtime();

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    _taskr->addProcessingUnit(processingUnit);
  }

  // Creating task functions
  auto taskAfc = computeManager.createExecutionUnit([_taskr]() { printf("Task A %lu\n", _taskr->getCurrentTask()->getLabel()); });
  auto taskBfc = computeManager.createExecutionUnit([_taskr]() { printf("Task B %lu\n", _taskr->getCurrentTask()->getLabel()); });
  auto taskCfc = computeManager.createExecutionUnit([_taskr]() { printf("Task C %lu\n", _taskr->getCurrentTask()->getLabel()); });

  // Now creating tasks and their dependency graph
  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto cTask = new taskr::Task(i * 3 + 2, taskCfc);
    cTask->addTaskDependency(i * 3 + 1);
    _taskr->addTask(cTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto bTask = new taskr::Task(i * 3 + 1, taskBfc);
    bTask->addTaskDependency(i * 3 + 0);
    _taskr->addTask(bTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto aTask = new taskr::Task(i * 3 + 0, taskAfc);
    if (i > 0) aTask->addTaskDependency(i * 3 - 1);
    _taskr->addTask(aTask);
  }

  // Running taskr
  _taskr->run();

  // Finalizing taskr
  delete _taskr;

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
