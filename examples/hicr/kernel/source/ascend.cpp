#include <filesystem>
#include <iomanip>
#include <stdio.h>
#include <acl/acl.h>
#include <backends/ascend/computationKernel.hpp>
#include <backends/ascend/kernel.hpp>
#include <backends/ascend/memoryKernel.hpp>
#include <backends/ascend/L0/executionUnit.hpp>
#include <backends/ascend/L0/processingUnit.hpp>
#include <backends/ascend/L1/memoryManager.hpp>
#include <backends/ascend/L1/topologyManager.hpp>
#include <backends/ascend/L1/communicationManager.hpp>
#include <backends/ascend/L1/computeManager.hpp>
#include <backends/sharedMemory/hwloc/L1/topologyManager.hpp>

#define BUFF_SIZE 192

namespace ascend = HiCR::backend::ascend;

void populateMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, float value)
{
  for (int i = 0; i < BUFF_SIZE; i++) ((aclFloat16 *)memorySlot->getPointer())[i] = aclFloatToFloat16(value);
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
    std::cout << "\n";
  }
}

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host topology manager
  HiCR::backend::sharedMemory::hwloc::L1::TopologyManager hostTopologyManager(&topology);
  hostTopologyManager.queryDevices();
  auto hostDevice = *hostTopologyManager.getDevices().begin();
  auto hostMemSpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;
  ascendTopologyManager.queryDevices();
  auto ascendDevice = *ascendTopologyManager.getDevices().begin();
  auto deviceMemSpace = *ascendDevice->getMemorySpaceList().begin();

  // Instantiating Ascend memory manager
  HiCR::backend::ascend::L1::MemoryManager ascendMemoryManager;

  // Allocate input and output buffers on both host and the device
  size_t size = BUFF_SIZE * sizeof(aclFloat16);
  auto input1Host = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto input1Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto input2Host = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto input2Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto outputHost = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto outputDevice = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  // Populate the input buffers with data
  populateMemorySlot(input1Host, 12.0);
  populateMemorySlot(input2Host, 2.0);

  // Instantiating Ascend communication manager
  HiCR::backend::ascend::L1::CommunicationManager ascendCommunicationManager;

  // Copy the inputs from the host buffers to the device buffers using a MemoryKernel abstraction
  ascend::MemoryKernel copyInput1MemoryKernel(&ascendCommunicationManager, input1Device, 0, input1Host, 0, size);
  ascend::MemoryKernel copyInput2MemoryKernel(&ascendCommunicationManager, input2Device, 0, input2Host, 0, size);

  // Create tensor descriptor (what's inside the tensor)
  const int64_t dims[] = {192, 1};
  auto tensorDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);

  if (tensorDesc == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Prepare kernel input tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> inputs({ascend::ComputationKernel::createTensorData(input1Device, tensorDesc),
                                                               ascend::ComputationKernel::createTensorData(input2Device, tensorDesc)});

  // Prepare kernel output tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> outputs({ascend::ComputationKernel::createTensorData(outputDevice, tensorDesc)});

  // Create the vector addition ComputeKernel
  auto currentPath = std::filesystem::current_path().string();
  auto kernelPath = currentPath + std::string("/../examples/hicr/kernel/op_models/0_Add_1_2_192_1_1_2_192_1_1_2_192_1.om");
  ascend::ComputationKernel kernel = ascend::ComputationKernel(kernelPath.c_str(), "Add", std::move(inputs), std::move(outputs), aclopCreateAttr());

  // copy the result back on the host
  ascend::MemoryKernel copyOutputMemoryKernel(&ascendCommunicationManager, outputHost, 0, outputDevice, 0, size);

  // create the stream of Kernel operations to be executed on the device
  std::vector<ascend::Kernel *> operations({&copyInput1MemoryKernel, &copyInput2MemoryKernel, &kernel, &copyOutputMemoryKernel});

  // Instantiating Ascend computation manager
  HiCR::backend::ascend::L1::ComputeManager ascendComputeManager;

  // Create execution unit
  auto executionUnit = ascendComputeManager.createExecutionUnit(operations);

  // Create a processing unit and initialize it with the desired device correct context
  auto ascendComputeResource = *ascendDevice->getComputeResourceList().begin();
  auto processingUnit = ascendComputeManager.createProcessingUnit(ascendComputeResource);
  processingUnit->initialize();

  // Create an execution state and initialize it
  auto executionState = ascendComputeManager.createExecutionState(executionUnit);

  // Execute the kernel stream
  processingUnit->start(std::move(executionState));

  // in the meantime we can check for completion
  // printf("Currently the kernel execution completion is %s\n", executionState.get()->checkFinalization() ? "true" : "false");

  // start teminating the processing unit
  processingUnit->terminate();

  // wait for termination
  processingUnit->await();

  // print the result
  printf("First vector contains: %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input1Host->getPointer())[0]));
  printf("Second vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input2Host->getPointer())[0]));
  printf("Vector sum is : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)outputHost->getPointer())[0]));

  // free memory slots
  ascendMemoryManager.freeLocalMemorySlot(input1Host);
  ascendMemoryManager.freeLocalMemorySlot(input1Device);
  ascendMemoryManager.freeLocalMemorySlot(input2Host);
  ascendMemoryManager.freeLocalMemorySlot(input2Device);
  ascendMemoryManager.freeLocalMemorySlot(outputHost);
  ascendMemoryManager.freeLocalMemorySlot(outputDevice);

  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  return 0;
}
