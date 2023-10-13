#include <cstdio>
#include <cstring>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <hicr/backends/sharedMemory/executionUnit.hpp>

#define ITERATIONS 100

// Singleton access for the taskr runtime
taskr::Runtime* _taskr;

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
  _taskr = new taskr::Runtime();

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    _taskr->addProcessingUnit(processingUnit);
  }

  // Now creating tasks and their dependency graph
  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto fc = new HiCR::backend::sharedMemory::ExecutionUnit([i]() { printf("Task C%lu\n", i); });
    auto cTask = new taskr::Task(i * 3 + 2, fc);
    cTask->addTaskDependency(i * 3 + 1);
    _taskr->addTask(cTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto fc = new HiCR::backend::sharedMemory::ExecutionUnit([i]() { printf("Task B%lu\n", i); });
    auto bTask = new taskr::Task(i * 3 + 1, fc);
    bTask->addTaskDependency(i * 3 + 0);
    _taskr->addTask(bTask);
  }

  for (size_t i = 0; i < ITERATIONS; i++)
  {
    auto fc = new HiCR::backend::sharedMemory::ExecutionUnit([i]() { printf("Task A%lu\n", i); });
    auto aTask = new taskr::Task(i * 3 + 0, fc);
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
