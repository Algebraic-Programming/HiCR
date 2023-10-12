#include <cstdio>
#include <cstring>
#include <chrono>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <workTask.hpp>

#define WORK_TASK_COUNT 100

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

  // Initializing taskr
  _taskr = new taskr::Runtime();

  // Getting the core subset from the argument list (could be from a file too)
  HiCR::Backend::computeResourceList_t coreSubset;
  for (int i = 1; i < argc; i++) coreSubset.insert(std::atoi(argv[i]));

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &coreId : coreSubset)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(coreId);

    // Assigning resource to the taskr
    _taskr->addProcessingUnit(processingUnit);
  }

  // Adding multiple compute tasks
  printf("Running %u work tasks with %lu cores...\n", WORK_TASK_COUNT, coreSubset.size());
  for (size_t i = 0; i < WORK_TASK_COUNT; i++) _taskr->addTask(new taskr::Task(i, &work));

  // Running taskr only on the core subset
  auto t0 = std::chrono::high_resolution_clock::now();
  _taskr->run();
  auto tf = std::chrono::high_resolution_clock::now();

  auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
  printf("Finished in %.3f seconds.\n", (double)dt * 1.0e-9);

  // Finalizing taskr
  delete _taskr;

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
