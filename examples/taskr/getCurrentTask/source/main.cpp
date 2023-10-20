#include <cstdio>
#include <cstring>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>

#define TASK_LABEL 42

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
  taskr::Runtime taskr;

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    taskr.addProcessingUnit(processingUnit);
  }

  // Creating task  execution unit
  auto taskExecutionUnit = computeManager.createExecutionUnit([&taskr]()
  {
   const auto hicrTask   = HiCR::getCurrentTask();
   const auto hicrWorker = HiCR::getCurrentWorker();
   const auto taskrLabel = taskr.getCurrentTask()->getLabel();
   printf("Current HiCR  Task   pointer:  0x%lX\n", (uint64_t)hicrTask);
   printf("Current HiCR  Worker pointer:  0x%lX\n", (uint64_t)hicrWorker);
   printf("Current TaskR Task   label:    %lu\n", taskrLabel);
  });

  // Creating a single task to print the internal references
  taskr.addTask(new taskr::Task(TASK_LABEL, taskExecutionUnit));

  // Running taskr
  taskr.run(&computeManager);

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
