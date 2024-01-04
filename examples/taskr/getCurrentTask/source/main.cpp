#include <cstdio>
#include <backends/sequential/L1/computeManager.hpp>
#include <backends/sequential/L1/topologyManager.hpp>
#include <frontends/taskr/runtime.hpp>
#include <frontends/taskr/task.hpp>

#define TASK_LABEL 42

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

// Initializing Sequential backend's topology manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

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
   printf("Current TaskR Task   label:    %lu\n", taskrLabel);
  });

  // Creating a single task to print the internal references
  taskr.addTask(new taskr::Task(TASK_LABEL, taskExecutionUnit));

  // Running taskr
  taskr.run(&computeManager);

  return 0;
}
