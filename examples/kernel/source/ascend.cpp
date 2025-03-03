#include <filesystem>
#include <memory>
#include <iomanip>
#include <stdio.h>
#include <acl/acl.h>
#include <hicr/backends/ascend/computationKernel.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/backends/ascend/memoryKernel.hpp>
#include <hicr/backends/ascend/L0/executionUnit.hpp>
#include <hicr/backends/ascend/L0/processingUnit.hpp>
#include <hicr/backends/ascend/L1/memoryManager.hpp>
#include <hicr/backends/ascend/L1/topologyManager.hpp>
#include <hicr/backends/ascend/L1/communicationManager.hpp>
#include <hicr/backends/ascend/L1/computeManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

#define BUFF_SIZE 192

namespace ascend = HiCR::backend::ascend;

void populateMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, float value)
{
  for (int i = 0; i < BUFF_SIZE; i++) ((aclFloat16 *)memorySlot->getPointer())[i] = aclFloatToFloat16(value);
}

ascend::ComputationKernel createComputeKernelFromFile(std::string                                           path,
                                                      std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                      std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                      aclopAttr                                            *kernelAttributes);

ascend::ComputationKernel createComputeKernelFromDirectory(std::string                                           path,
                                                           std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                           std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                           aclopAttr                                            *kernelAttributes);

void executeKernel(std::shared_ptr<HiCR::L0::Device> ascendDevice, std::vector<std::shared_ptr<ascend::Kernel>> operations);

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
  HiCR::backend::hwloc::L1::TopologyManager hostTopologyManager(&topology);
  auto                                      hostTopology = hostTopologyManager.queryTopology();
  auto                                      hostDevice   = *hostTopology.getDevices().begin();
  auto                                      hostMemSpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;
  auto                                       ascendTopology = ascendTopologyManager.queryTopology();
  auto                                       ascendDevice   = *ascendTopology.getDevices().begin();
  auto                                       deviceMemSpace = *ascendDevice->getMemorySpaceList().begin();

  // Instantiating Ascend memory manager
  HiCR::backend::ascend::L1::MemoryManager ascendMemoryManager;

  // Allocate input and output buffers on both host and the device
  size_t size         = BUFF_SIZE * sizeof(aclFloat16);
  auto   input1Host   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto   input1Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto input2Host   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto input2Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  auto outputHost   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, size);
  auto outputDevice = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, size);

  // Populate the input buffers with data
  populateMemorySlot(input1Host, 12.0);
  populateMemorySlot(input2Host, 2.0);

  // Instantiating Ascend communication manager
  HiCR::backend::ascend::L1::CommunicationManager ascendCommunicationManager;

  // Copy the inputs from the host buffers to the device buffers using a MemoryKernel abstraction
  auto copyInput1MemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, input1Device, 0, input1Host, 0, size);
  auto copyInput2MemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, input2Device, 0, input2Host, 0, size);

  // Copy the result back on the host using a MemoryKernel abstraction
  auto copyOutputMemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, outputHost, 0, outputDevice, 0, size);

  // Create tensor descriptor (what's inside the tensor). In this example it is the same for all tensors
  const int64_t dims[]           = {192, 1};
  auto          tensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);
  if (tensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Create kernel attributes
  auto kernelAttributes = aclopCreateAttr();
  if (kernelAttributes == NULL) HICR_THROW_RUNTIME("Can not create kernel attributes");

  // Prepare kernel input tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> inputs(
    {ascend::ComputationKernel::createTensorData(input1Device, tensorDescriptor), ascend::ComputationKernel::createTensorData(input2Device, tensorDescriptor)});

  // Prepare kernel output tensor data
  std::vector<ascend::ComputationKernel::tensorData_t> outputs({ascend::ComputationKernel::createTensorData(outputDevice, tensorDescriptor)});

  // Create the ComputationKernel by reading it from file
  auto fileComputationKernel = std::make_shared<ascend::ComputationKernel>(
    createComputeKernelFromFile("/../examples/kernel/op_models/0_Add_1_2_192_1_1_2_192_1_1_2_192_1.om", inputs, outputs, kernelAttributes));

  // Create the stream of Kernel operations to be executed on the device
  auto operations = std::vector<std::shared_ptr<ascend::Kernel>>{copyInput1MemoryKernel, copyInput2MemoryKernel, fileComputationKernel, copyOutputMemoryKernel};

  // Execute the stream of Kernels
  executeKernel(ascendDevice, operations);

  // Print the result
  printf("First vector contains: %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input1Host->getPointer())[0]));
  printf("Second vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input2Host->getPointer())[0]));
  printf("Vector sum is : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)outputHost->getPointer())[0]));

  // Reset output tensor
  populateMemorySlot(outputHost, 0.0);

  // Create the ComputationKernel by looking up in a directory
  auto directoryComputationKernel =
    std::make_shared<ascend::ComputationKernel>(createComputeKernelFromDirectory("/../examples/kernel/op_models", inputs, outputs, kernelAttributes));

  // Create the stream of Kernel operations to be executed on the device
  operations = std::vector<std::shared_ptr<ascend::Kernel>>{copyInput1MemoryKernel, copyInput2MemoryKernel, directoryComputationKernel, copyOutputMemoryKernel};

  // Execute the stream of Kernels
  executeKernel(ascendDevice, operations);

  // Print the result
  printf("First vector contains: %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input1Host->getPointer())[0]));
  printf("Second vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input2Host->getPointer())[0]));
  printf("Vector sum is : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)outputHost->getPointer())[0]));

  // Free memory slots
  ascendMemoryManager.freeLocalMemorySlot(input1Host);
  ascendMemoryManager.freeLocalMemorySlot(input1Device);
  ascendMemoryManager.freeLocalMemorySlot(input2Host);
  ascendMemoryManager.freeLocalMemorySlot(input2Device);
  ascendMemoryManager.freeLocalMemorySlot(outputHost);
  ascendMemoryManager.freeLocalMemorySlot(outputDevice);

  // Destroy tensor descriptors and kernel attributes
  aclDestroyTensorDesc(tensorDescriptor);
  aclopDestroyAttr(kernelAttributes);

  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  return 0;
}

ascend::ComputationKernel createComputeKernelFromFile(std::string                                           path,
                                                      std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                      std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                      aclopAttr                                            *kernelAttributes)
{
  auto currentPath = std::filesystem::current_path().string();
  auto kernelPath  = currentPath + std::string(path);

  // Instantiate a ComputationKernel abstraction by providing a path to an .om file. The kernel is loaded internally
  auto kernel = ascend::ComputationKernel(kernelPath.c_str(), "Add", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}

ascend::ComputationKernel createComputeKernelFromDirectory(std::string                                           path,
                                                           std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                           std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                           aclopAttr                                            *kernelAttributes)
{
  aclError err;

  auto currentPath = std::filesystem::current_path().string();
  auto kernelPath  = currentPath + std::string(path);

  // Set the directory in which ACL will performs the lookup for kernels
  err = aclopSetModelDir(kernelPath.c_str());
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set the model directory %s in ACL runtime. Error: %d", kernelPath.c_str(), err);

  // Instantiate a ComputationKernel abstraction by providing only its features. The kernel has been already loaded in aclopSetModelDir()
  auto kernel = ascend::ComputationKernel("Add", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}

void executeKernel(std::shared_ptr<HiCR::L0::Device> ascendDevice, std::vector<std::shared_ptr<ascend::Kernel>> operations)
{
  // Instantiating Ascend computation manager
  HiCR::backend::ascend::L1::ComputeManager ascendComputeManager;

  // Create execution unit
  auto executionUnit = ascendComputeManager.createExecutionUnit(operations);

  // Create a processing unit and initialize it with the desired device correct context
  auto ascendComputeResource = *ascendDevice->getComputeResourceList().begin();
  auto processingUnit        = ascendComputeManager.createProcessingUnit(ascendComputeResource);
  ascendComputeManager.initialize(processingUnit);

  // Create an execution state and initialize it
  auto executionState = ascendComputeManager.createExecutionState(executionUnit);

  // Execute the kernel stream
  ascendComputeManager.start(processingUnit, executionState);

  // start teminating the processing unit
  ascendComputeManager.terminate(processingUnit);

  // wait for termination
  ascendComputeManager.await(processingUnit);
}