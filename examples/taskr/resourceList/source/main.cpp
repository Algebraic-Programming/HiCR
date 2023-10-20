#include <cstdio>
#include <cstring>
#include <chrono>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <workTask.hpp>

#define WORK_TASK_COUNT 100

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
  taskr::Runtime taskr;

  // Getting the core subset from the argument list (could be from a file too)
  std::set<int> coreSubset;
  for (int i = 1; i < argc; i++) coreSubset.insert(std::atoi(argv[i]));

  // Sanity check
  if (coreSubset.empty()) { fprintf(stderr, "Launch error: no compute resources provided\n"); exit(-1); }

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &coreId : coreSubset)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(coreId);

    // Assigning resource to the taskr
    taskr.addProcessingUnit(processingUnit);
  }

  // Creating task  execution unit
  auto taskExecutionUnit = computeManager.createExecutionUnit([&taskr](){work();});

  // Adding multiple compute tasks
  printf("Running %u work tasks with %lu processing units...\n", WORK_TASK_COUNT, coreSubset.size());
  for (size_t i = 0; i < WORK_TASK_COUNT; i++) taskr.addTask(new taskr::Task(i, taskExecutionUnit));

  // Running taskr only on the core subset
  auto t0 = std::chrono::high_resolution_clock::now();
  taskr.run(&computeManager);
  auto tf = std::chrono::high_resolution_clock::now();

  auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
  printf("Finished in %.3f seconds.\n", (double)dt * 1.0e-9);

  // Freeing up memory
  delete taskExecutionUnit;
  hwloc_topology_destroy(topology);

  return 0;
}
