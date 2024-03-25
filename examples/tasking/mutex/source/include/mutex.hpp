#include <cstdio>
#include <cassert>
#include <hicr/L0/device.hpp>
#include <backends/host/L1/computeManager.hpp>
#include "../runtime.hpp"
 
#define _CONCURRENT_TASKS 10000ul

void mutex(HiCR::backend::host::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing taskr
  taskr::Runtime taskr;

  // Assigning processing Re to TaskR
  for (const auto &computeResource : computeResources) taskr.addProcessingUnit(std::move(computeManager->createProcessingUnit(computeResource)));

  // Contention value
  size_t value = 0;

  // Task-aware mutex
  HiCR::tasking::Mutex m;

  // Creating task function
  auto taskfc = computeManager->createExecutionUnit([&]() { m.lock(); value++; m.unlock(); });

  // Running concurrent tasks
  for (size_t i = 0; i < _CONCURRENT_TASKS; i++) taskr.addTask(new HiCR::tasking::Task(i, taskfc));

  // Running taskr
  taskr.run(computeManager);

  // Value should be equal to concurrent task count
  printf("Value %lu / Expected %lu\n", value, _CONCURRENT_TASKS);
  assert(value == _CONCURRENT_TASKS);
}
