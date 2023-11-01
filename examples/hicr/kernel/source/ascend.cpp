#include <hicr.hpp>
#include <hicr/backends/ascend/computeManager.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <iomanip>
#include <stdio.h>

#define BUFF_SIZE 192

namespace ascend = HiCR::backend::ascend;

void populateMemorySlot(HiCR::MemorySlot *memorySlot, float value)
{
  for (int i = 0; i < BUFF_SIZE; i++)
  {
    ((aclFloat16 *)memorySlot->getPointer())[i] = aclFloatToFloat16(value);
  }
}

void doPrintMatrix(const aclFloat16 *matrix, uint32_t numRows, uint32_t numCols)
{
  uint32_t rows = numRows;

  for (uint32_t i = 0; i < rows; ++i)
  {
    for (uint32_t j = 0; j < numCols; ++j)
    {
      std::cout << std::setw(10) << aclFloat16ToFloat(matrix[i * numCols + j]);
    }
    std::cout << std::endl;
  }
}

int main(int argc, char **argv)
{
  aclError err;
  err = aclInit(NULL);

  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  printf("create mem manager\n");
  // Need also memory manager to perform the memory allocation on the device
  ascend::MemoryManager memoryManager;

  printf("query mem spaces\n");
  // get memory spaces
  memoryManager.queryMemorySpaces();
  auto memorySpaces = memoryManager.getMemorySpaceList();

  printf("alloc input1\n");
  size_t size = BUFF_SIZE * sizeof(aclFloat16);
  auto input1Host = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  auto input1Device = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  printf("alloc input2\n");
  auto input2Host = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  auto input2Device = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  printf("alloc output\n");
  auto outputHost = memoryManager.allocateLocalMemorySlot(*memorySpaces.rbegin(), size);
  auto outputDevice = memoryManager.allocateLocalMemorySlot(*memorySpaces.begin(), size);

  printf("populate mem slots\n");
  populateMemorySlot(input1Host, 12.0);
  populateMemorySlot(input2Host, 2.0);

  printf("copy data to ascend\n");
  memoryManager.memcpy(input1Device, 0, input1Host, 0, size);
  memoryManager.memcpy(input2Device, 0, input2Host, 0, size);

  printf("Create compute manager\n");
  // Instantiating Ascend backend
  ascend::ComputeManager computeManager;

  // Creating compute unit

  printf("init input data\n");

  auto castedInput1Device = dynamic_cast<ascend::MemorySlot *>(input1Device);
  auto castedInput2Device = dynamic_cast<ascend::MemorySlot *>(input2Device);
  auto castedOutputDevice = dynamic_cast<ascend::MemorySlot *>(outputDevice);

  // prepare kernel input and output data descriptor
  std::vector<ascend::ExecutionUnit::tensorData_t> inputs = std::vector<ascend::ExecutionUnit::tensorData_t>(
    {
      ascend::ExecutionUnit::tensorData_t{
        .memorySlot = castedInput1Device,
        .dimensions = std::vector<int64_t>{192, 1},
        .DataType = ACL_FLOAT16,
        .format = ACL_FORMAT_ND,
      },
      ascend::ExecutionUnit::tensorData_t{
        .memorySlot = castedInput2Device,
        .dimensions = std::vector<int64_t>{192, 1},
        .DataType = ACL_FLOAT16,
        .format = ACL_FORMAT_ND,
      },
    });

  std::vector<ascend::ExecutionUnit::tensorData_t> outputs = std::vector<ascend::ExecutionUnit::tensorData_t>(
    {
      ascend::ExecutionUnit::tensorData_t{
        .memorySlot = castedOutputDevice,
        .dimensions = std::vector<int64_t>{192, 1},
        .DataType = ACL_FLOAT16,
        .format = ACL_FORMAT_ND,
      },
    });

  printf("create exec unit\n");
  // Create execution unit
  // probably configure data buffer and tensor descriptors
  auto executionUnit = computeManager.createExecutionUnit((const char *)"/home/HwHiAiUser/hicr/examples/hicr/kernel/op_models/0_Add_1_2_192_1_1_2_192_1_1_2_192_1.om", std::move(inputs), std::move(outputs), aclopCreateAttr());

  // Query compute resources and get them
  computeManager.queryComputeResources();
  auto computeResources = computeManager.getComputeResourceList();

  printf("create processing unit\n");
  // Create a processing unit and initialize it
  // create processing unit with correct context
  auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());
  processingUnit->initialize();

  printf("create exec state\n");
  // Create an execution state and initialize it
  auto executionState = computeManager.createExecutionState();
  printf("init exec state\n");
  // create the stream
  executionState->initialize(executionUnit);

  // // Execute the kernel
  printf("start exec state\n");
  processingUnit->start(std::move(executionState));

  printf("memcpy result\n");
  memoryManager.memcpy(outputHost, 0, outputDevice, 0, size);

  printf("print result\n");
  doPrintMatrix((const aclFloat16 *)outputHost->getPointer(), 1, 192);
  printf("the end \n");
  return 0;
}
