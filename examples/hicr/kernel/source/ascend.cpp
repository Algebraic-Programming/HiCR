#include <hicr.hpp>
#include <hicr/backends/ascend/computeManager.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <stdio.h>

#define BUFF_SIZE 192

namespace ascend = HiCR::backend::ascend;

void populateMemorySlot(HiCR::MemorySlot *memorySlot, float value)
{
  for (int i = 0; i < BUFF_SIZE; i++)
  {
    ((aclFloat16 *) memorySlot->getPointer())[i] = 0.0;
  }
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Not enough arguments\n");
    return 1;
  }

  // // Need also memory manager to perform the memory allocation on the device
  // ascend::MemoryManager memoryManager;

  // // get memory spaces
  // memoryManager.queryMemorySpaces();
  // auto memorySpaces = memoryManager.getMemorySpaceList();

  // size_t size = BUFF_SIZE * sizeof(aclFloat16);
  // auto input1Host = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  // auto input1Device = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  // auto input2Host = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  // auto input2Device = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  // auto outputHost = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  // auto outputDevice = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  // populateMemorySlot(input1Host, 12.0);
  // populateMemorySlot(input2Host, 2.0);

  // memoryManager.memcpy(input1Device, 0, input1Host, 0, size);
  // memoryManager.memcpy(input2Device, 0, input2Host, 0, size);

  // Instantiating Ascend backend
  ascend::ComputeManager computeManager;

  // // Creating compute unit

  // prepare kernel input and output data descriptor
  std::vector<ascend::ExecutionUnit::DataIO> inputs = std::vector<ascend::ExecutionUnit::DataIO>();

  // // inputs[0] = ascend::ExecutionUnit::DataIO{.DataType=ACL_FLOAT16, .dimensions=std::vector({BUFF_SIZE, 1}), .format=ACL_FORMAT_NC, .ptr= }

  std::vector<ascend::ExecutionUnit::DataIO> outputs = std::vector<ascend::ExecutionUnit::DataIO>();

  // Create execution unit
  auto executionUnit = computeManager.createExecutionUnit((const char *)argv[1], std::move(inputs), std::move(outputs));

  // // Query compute resources and get them
  // computeManager.queryComputeResources();
  // auto computeResources = computeManager.getComputeResourceList();

  // // Create a processing unit and initialize it
  // auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());
  // processingUnit->initialize();

  // // Create an execution state and initialize it
  // auto executionState = computeManager.createExecutionState();
  // executionState->initialize(executionUnit);

  // // Execute the kernel
  // processingUnit->start(std::move(executionState));

  // memoryManager.memcpy(outputHost, 0, outputDevice, 0, size);

  return 0;
}
