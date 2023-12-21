#include <backends/ascend/L0/executionUnit.hpp>
#include <backends/ascend/L0/processingUnit.hpp>
#include <backends/ascend/L1/computeManager.hpp>
#include <backends/ascend/L1/memoryManager.hpp>
#include <backends/ascend/computationKernel.hpp>
#include <backends/ascend/kernel.hpp>
#include <backends/ascend/memoryKernel.hpp>
#include <filesystem>
#include <iomanip>
#include <stdio.h>

#define BUFF_SIZE 192

namespace ascend = HiCR::backend::ascend;

void populateMemorySlot(HiCR::L0::MemorySlot *memorySlot, float value)
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
    std::cout << "\n";
  }
}

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  aclError err = aclInit(_configPath);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  //////////////////////// This part of the code will be greatly simplified once we add the device class 

  // Instantiating Memory Manager
  ascend::L1::MemoryManager memoryManager(ascendCore);

  // Asking the memory manager to check the available resources
  memoryManager.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = memoryManager.getMemorySpaceList();

  // Pointers of the intervening memory spaces 
  ascend::L0::MemorySpace* deviceMemSpace;
  ascend::L0::MemorySpace* hostMemSpace;

  // Get all memory spaces associated with Ascend devices and the one associated with the host
  std::vector<ascend::L0::MemorySpace*> deviceMemSpaces; 
  for (const auto memSpace : memSpaces)
  {
    // Getting ascend type memory space pointer
    auto ascendMemSpace = (ascend::L0::MemorySpace*) memSpace;

    // Adding memory spaces that are not of host type
    if (ascendMemSpace->getDeviceType() != ascend::deviceType_t::Host) deviceMemSpaces.push_back(ascendMemSpace);
  
    // Adding memory spaces is of host type, set it
    if (ascendMemSpace->getDeviceType() == ascend::deviceType_t::Host) hostMemSpace = ascendMemSpace;
  }
  // make memory spaces contain only device ids
  deviceMemSpace = *deviceMemSpaces.begin();

  
  // Instantiating Memory Manager
  ascend::L1::ComputeManager computeManager(ascendCore);

  // Asking the memory manager to check the available resources
  computeManager.queryComputeResources();

  // Obtaining memory spaces
  auto computeResources = computeManager.getComputeResourceList();

  // Pointers of the intervening memory spaces 
  ascend::L0::ComputeResource* deviceComputeResource;

  // Get all memory spaces associated with Ascend devices and the one associated with the host
  std::vector<ascend::L0::ComputeResource*> deviceComputeResources; 
  for (const auto computeResource : computeResources)
  {
    // Getting ascend type memory space pointer
    auto ascendComputeResource = (ascend::L0::ComputeResource*) computeResource;

    // Adding memory spaces that are not of host type
    if (ascendComputeResource->getDeviceType() != ascend::deviceType_t::Host) deviceComputeResources.push_back(ascendComputeResource);
  }
  
  // make memory spaces contain only device ids
  deviceComputeResource = *deviceComputeResources.begin();

  //////////////////////// This part of the code will be greatly simplified once we add the device class 

  // Allocate input and output buffers on both host and the device
  size_t size = BUFF_SIZE * sizeof(aclFloat16);
  auto input1Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto input1Device = memoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto input2Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto input2Device = memoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto outputHost = memoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto outputDevice = memoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  // Populate the input buffers with data
  populateMemorySlot(input1Host, 12.0);
  populateMemorySlot(input2Host, 2.0);

  // Copy the inputs from the host buffers to the device buffers using a MemoryKernel abstraction
  ascend::MemoryKernel copyInput1MemoryKernel = ascend::MemoryKernel(&memoryManager, input1Device, 0, input1Host, 0, size);
  ascend::MemoryKernel copyInput2MemoryKernel = ascend::MemoryKernel(&memoryManager, input2Device, 0, input2Host, 0, size);

  // Create the tensor data structure for the ComputeKernel
  auto castedInput1Device = static_cast<ascend::L0::MemorySlot *>(input1Device);
  auto castedInput2Device = static_cast<ascend::L0::MemorySlot *>(input2Device);
  auto castedOutputDevice = static_cast<ascend::L0::MemorySlot *>(outputDevice);
  if (castedInput1Device == NULL) HICR_THROW_RUNTIME("Can not perform cast on memory slot");
  if (castedInput2Device == NULL) HICR_THROW_RUNTIME("Can not perform cast on memory slot");
  if (castedOutputDevice == NULL) HICR_THROW_RUNTIME("Can not perform cast on memory slot");

  // Create tensor descriptor (what's inside the tensor)
  const int64_t dims[] = {192, 1};
  auto tensorDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);

  if (tensorDesc == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Prepare kernel input tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> inputs = std::vector<ascend::ComputationKernel::tensorData_t>(
    {
      ascend::ComputationKernel::tensorData_t{
        .dataBuffer = castedInput1Device->getDataBuffer(),
        .tensorDescriptor = tensorDesc},
      ascend::ComputationKernel::tensorData_t{
        .dataBuffer = castedInput2Device->getDataBuffer(),
        .tensorDescriptor = tensorDesc},
    });

  // Prepare kernel output tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> outputs = std::vector<ascend::ComputationKernel::tensorData_t>(
    {
      ascend::ComputationKernel::tensorData_t{
        .dataBuffer = castedOutputDevice->getDataBuffer(),
        .tensorDescriptor = tensorDesc,
      },
    });

  // Create the vector addition ComputeKernel
  auto currentPath = std::filesystem::current_path().string();
  auto kernelPath = currentPath + std::string("/../examples/hicr/kernel/op_models/0_Add_1_2_192_1_1_2_192_1_1_2_192_1.om");
  ascend::ComputationKernel kernel = ascend::ComputationKernel(
    kernelPath.c_str(),
    "Add",
    std::move(inputs),
    std::move(outputs),
    aclopCreateAttr());

  // copy the result back on the host
  ascend::MemoryKernel copyOutputMemoryKernel = ascend::MemoryKernel(&memoryManager, outputHost, 0, outputDevice, 0, size);

  // create the stream of Kernel operations to be executed on the device
  std::vector<ascend::Kernel *> operations = std::vector<ascend::Kernel *>(
    {&copyInput1MemoryKernel,
     &copyInput2MemoryKernel,
     &kernel,
     &copyOutputMemoryKernel});

  // Create execution unit
  auto executionUnit = computeManager.createExecutionUnit(operations);

  // Create a processing unit and initialize it with the desired device correct context
  auto processingUnit = computeManager.createProcessingUnit(deviceComputeResource);
  processingUnit->initialize();

  // Create an execution state and initialize it
  auto executionState = processingUnit->createExecutionState(executionUnit);

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
  memoryManager.freeLocalMemorySlot(input1Host);
  memoryManager.freeLocalMemorySlot(input1Device);
  memoryManager.freeLocalMemorySlot(input2Host);
  memoryManager.freeLocalMemorySlot(input2Device);
  memoryManager.freeLocalMemorySlot(outputHost);
  memoryManager.freeLocalMemorySlot(outputDevice);

  aclError err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);
  
  return 0;
}
