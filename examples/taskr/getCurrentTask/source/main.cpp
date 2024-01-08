#include <cstdio>
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <frontends/taskr/runtime.hpp>
#include <frontends/taskr/task.hpp>

#define TASK_LABEL 42

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing Pthread-based compute manager to run tasks in parallel
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager topologyManager(&topology);

  // Asking backend to check the available devices
  topologyManager.queryDevices();

  // Getting first device found
  auto d = *topologyManager.getDevices().begin();

  // Updating the compute resource list
  auto computeResources = d->getComputeResourceList();

  // Initializing taskr
  taskr::Runtime taskr;

  // Create processing units from the detected compute resource list and giving them to taskr
  for (auto resource : computeResources)
  {
    // Creating a processing unit out of the computational resource
    auto processingUnit = computeManager.createProcessingUnit(resource);

    // Assigning resource to the taskr
    taskr.addProcessingUnit(std::move(processingUnit));
  }

  // Creating task  execution unit
  auto taskExecutionUnit = computeManager.createExecutionUnit([&taskr]()
                                                              {
   // Printing associated task label
   const auto taskrLabel = taskr.getCurrentTask()->getLabel();
   printf("Current TaskR Task   label:    %lu\n", taskrLabel); });

  // Creating a single task to print the internal references
  taskr.addTask(new taskr::Task(TASK_LABEL, taskExecutionUnit));

  // Running taskr
  taskr.run(&computeManager);

  return 0;
}
