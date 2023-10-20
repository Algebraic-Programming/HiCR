#include <cstdio>
#include <cstring>
#include <hwloc.h>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/computeManager.hpp>

void workFc()
{
 __volatile__ double value = 2.0;
 for (size_t i = 0; i < 5000; i++)
  for (size_t j = 0; j < 5000; j++)
  {
    value = sqrt(value + i);
    value = value * value;
  }
}

void waitFc(taskr::Runtime* taskr, size_t secondsDelay)
{
 // Reducing maximum active workers to 1
 taskr->setMaximumActiveWorkers(1);

 printf("Starting long task...\n");
 fflush(stdout);
 sleep(secondsDelay);
 printf("Finished long task...\n");
 fflush(stdout);

 // Increasing maximum active workers
 taskr->setMaximumActiveWorkers(1024);
}

int main(int argc, char **argv)
{
  // Getting arguments, if provided
  size_t workTaskCount = 1000;
  size_t secondsDelay = 5;
  if (argc > 1) workTaskCount = std::atoi(argv[1]);
  if (argc > 2) secondsDelay = std::atoi(argv[2]);

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

  // Creating task work execution unit
  auto workExecutionUnit = computeManager.createExecutionUnit([]() { workFc(); });

  // Creating task wait execution unit
  auto waitExecutionUnit = computeManager.createExecutionUnit([&taskr, &secondsDelay]() { waitFc(&taskr, secondsDelay); });

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    taskr.addProcessingUnit(processingUnit);
  }

  printf("Starting many work tasks...\n");
  fflush(stdout);

  // Building task graph. First a lot of pure work tasks.
  for (size_t i = 0; i < workTaskCount; i++)
  {
    auto workTask = new taskr::Task(i, workExecutionUnit);
    taskr.addTask(workTask);
  }

  // Then creating a single wait task that suspends all workers except for one
  auto waitTask = new taskr::Task(workTaskCount + 1, waitExecutionUnit);
  for (size_t i = 0; i < workTaskCount; i++) waitTask->addTaskDependency(i);
  taskr.addTask(waitTask);

  // Then creating another batch of work tasks
  for (size_t i = 0; i < workTaskCount; i++)
  {
    auto workTask = new taskr::Task(workTaskCount + i + 1, workExecutionUnit);
    workTask->addTaskDependency(workTaskCount + 1);
    taskr.addTask(workTask);
  }

  // Running taskr
  taskr.run(&computeManager);

  printf("Finished all tasks.\n");

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
