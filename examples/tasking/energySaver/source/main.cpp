#include <cstdio>
#include <hwloc.h>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>
#include "../runtime.hpp"

void workFc(const size_t iterations)
{
  __volatile__ double value = 2.0;
  for (size_t i = 0; i < iterations; i++)
    for (size_t j = 0; j < iterations; j++)
    {
      value = sqrt(value + i);
      value = value * value;
    }
}

void waitFc(Runtime *runtime, size_t secondsDelay)
{
  // Reducing maximum active workers to 1
  runtime->setMaximumActiveWorkers(1);

  printf("Starting long task...\n");
  fflush(stdout);

  sleep(secondsDelay);

  printf("Finished long task...\n");
  fflush(stdout);

  // Increasing maximum active workers
  runtime->setMaximumActiveWorkers(1024);
}

int main(int argc, char **argv)
{
  // Getting arguments, if provided
  size_t workTaskCount = 1000;
  size_t secondsDelay  = 5;
  size_t iterations    = 5000;
  if (argc > 1) workTaskCount = std::atoi(argv[1]);
  if (argc > 2) secondsDelay = std::atoi(argv[2]);
  if (argc > 3) iterations = std::atoi(argv[3]);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Pthreads-based compute manager to run tasks in parallel
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing runtime
  Runtime runtime;

  // Creating task work execution unit
  auto workExecutionUnit = computeManager.createExecutionUnit([&iterations]() { workFc(iterations); });

  // Creating task wait execution unit
  auto waitExecutionUnit = computeManager.createExecutionUnit([&runtime, &secondsDelay]() { waitFc(&runtime, secondsDelay); });

  // Create processing units from the detected compute resource list and giving them to runtime
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the runtime
    runtime.addProcessingUnit(std::move(processingUnit));
  }

  printf("Starting many work tasks...\n");
  fflush(stdout);

  // Building task graph. First a lot of pure work tasks.
  for (size_t i = 0; i < workTaskCount; i++)
  {
    auto workTask = new Task(i, workExecutionUnit);
    runtime.addTask(workTask);
  }

  // Then creating a single wait task that suspends all workers except for one
  auto waitTask = new Task(workTaskCount + 1, waitExecutionUnit);
  for (size_t i = 0; i < workTaskCount; i++) waitTask->addTaskDependency(i);
  runtime.addTask(waitTask);

  // Then creating another batch of work tasks
  for (size_t i = 0; i < workTaskCount; i++)
  {
    auto workTask = new Task(workTaskCount + i + 1, workExecutionUnit);
    workTask->addTaskDependency(workTaskCount + 1);
    runtime.addTask(workTask);
  }

  // Running runtime
  runtime.run(&computeManager);

  printf("Finished all tasks.\n");

  // Freeing up memory
  hwloc_topology_destroy(topology);

  return 0;
}
