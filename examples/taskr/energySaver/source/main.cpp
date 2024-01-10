#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <cstdio>
#include <frontends/taskr/runtime.hpp>
#include <frontends/taskr/task.hpp>
#include <hwloc.h>

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

void waitFc(taskr::Runtime *taskr, size_t secondsDelay)
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
  size_t iterations = 5000;
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
  HiCR::backend::host::hwloc::L1::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing taskr
  taskr::Runtime taskr;

  // Creating task work execution unit
  auto workExecutionUnit = computeManager.createExecutionUnit([&iterations]()
                                                              { workFc(iterations); });

  // Creating task wait execution unit
  auto waitExecutionUnit = computeManager.createExecutionUnit([&taskr, &secondsDelay]()
                                                              { waitFc(&taskr, secondsDelay); });

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto &resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    taskr.addProcessingUnit(std::move(processingUnit));
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
