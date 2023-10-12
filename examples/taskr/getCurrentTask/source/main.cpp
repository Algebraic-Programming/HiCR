#include <cstdio>
#include <cstring>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>

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

  // Single task printing the pointer to
  _taskr->addTask(new taskr::Task(0, []()
    {
     auto hicrTask   = HiCR::getCurrentTask();
     auto hicrWorker = HiCR::getCurrentWorker();
     printf("Current HiCR Task pointer:   0x%lX\n", (uint64_t)hicrTask);
     printf("Current HiCR Worker pointer: 0x%lX\n", (uint64_t)hicrWorker);
    }
  ));

  // Running taskr
  _taskr->run();

  // Finalizing taskr
  delete _taskr;

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
