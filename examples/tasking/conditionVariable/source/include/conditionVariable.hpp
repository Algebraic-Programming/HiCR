#include <cstdio>
#include <cassert>
#include <thread>
#include <chrono>
#include <hicr/L0/device.hpp>
#include <backends/host/L1/computeManager.hpp>
#include "../runtime.hpp"
 
using namespace std::chrono_literals;
#define _INITIAL_VALUE 7ul

void conditionVariable(HiCR::backend::host::L1::ComputeManager *computeManager, const HiCR::L0::Device::computeResourceList_t &computeResources)
{
  // Initializing taskr
  taskr::Runtime taskr;

  // Assigning processing Re to TaskR
  for (const auto &computeResource : computeResources) taskr.addProcessingUnit(std::move(computeManager->createProcessingUnit(computeResource)));

  // Contention value
  size_t value = 0;

  // Task-aware mutex
  HiCR::tasking::ConditionVariable cv;

  // Creating task functions
  auto thread1Fc = computeManager->createExecutionUnit([&]()
   {
     std::this_thread::sleep_for(500ms);
     value = 7;
     cv.notifyOne();
     cv.wait();
     value = value + 7;
   });

  auto thread2Fc = computeManager->createExecutionUnit([&]()
  {
    cv.wait();
    value = value + 7;
    std::this_thread::sleep_for(500ms);
    cv.notifyOne();
  });

  taskr.addTask(new HiCR::tasking::Task(0, thread1Fc));
  taskr.addTask(new HiCR::tasking::Task(1, thread2Fc));

  // Running taskr
  taskr.run(computeManager);

  // Value should be equal to concurrent task count
  size_t expectedValue = _INITIAL_VALUE * 3;
  printf("Value %lu / Expected %lu\n", value, expectedValue);
  assert(value == expectedValue);
}
