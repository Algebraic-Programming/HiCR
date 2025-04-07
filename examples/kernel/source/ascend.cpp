/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filesystem>
#include <memory>
#include <iomanip>
#include <stdio.h>
#include <acl/acl.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/ascend/computationKernel.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/backends/ascend/memoryKernel.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/topologyManager.hpp>
#include <hicr/backends/ascend/communicationManager.hpp>
#include <hicr/backends/ascend/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "./include/kernel.hpp"

#define A 128
#define B 64
#define C 256

namespace ascend = HiCR::backend::ascend;

/**
 * Populate a matrix contained in a memory slot with the desired value converted to aclFloat16
 * 
 * @param[inout] memorySlot memory slot containing the matrix
 * @param[in] rows 
 * @param[in] columns
 * @param[in] value 
*/
void populateMemorySlot(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot, int rows, int columns, float value)
{
  for (int i = 0; i < rows * columns; i++) { ((aclFloat16 *)memorySlot->getPointer())[i] = aclFloatToFloat16(value); }
}

/**
 * Create a Compute Kernel from a single .om file
 * 
 * @param[in] path the absolute path to the .om file
 * @param[in] inputs input tensor related data structure
 * @param[in] outputs output tensor related data structure
 * @param[in] kernelAttributes kernel attributes
 * 
 * @return the ascend compute kernel
*/
std::shared_ptr<ascend::ComputationKernel> createComputeKernelFromFile(const std::string                                    &path,
                                                                       std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                                       std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                                       const aclopAttr                                      *kernelAttributes);

/**
 * Create a Compute Kernel from a single .om file
 * 
 * @param[in] path the absolute path to the folder containing the applications .om files
 * @param[in] inputs input tensor related data structure
 * @param[in] outputs output tensor related data structure
 * @param[in] kernelAttributes kernel attributes
 * 
 * @return the ascend compute kernel
*/
std::shared_ptr<ascend::ComputationKernel> createComputeKernelFromDirectory(const std::string                                    &path,
                                                                            std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                                            std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                                            const aclopAttr                                      *kernelAttributes);

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Creating HWloc topology object
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc and ascend
  // Initializing HWLoc-based host topology manager and retrieve host memory space
  HiCR::backend::hwloc::TopologyManager hostTopologyManager(&topology);
  auto                                  hostTopology = hostTopologyManager.queryTopology();
  auto                                  hostDevice   = *hostTopology.getDevices().begin();
  auto                                  hostMemSpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing ascend topology manager and retrieve memory space and compute resource of one of the devices
  HiCR::backend::ascend::TopologyManager ascendTopologyManager;
  auto                                   ascendTopology        = ascendTopologyManager.queryTopology();
  auto                                   ascendDevice          = *ascendTopology.getDevices().begin();
  auto                                   deviceMemSpace        = *ascendDevice->getMemorySpaceList().begin();
  auto                                   deviceComputeResource = *ascendDevice->getComputeResourceList().begin();

  // Instantiating Ascend memory, compute, and communication manager
  HiCR::backend::ascend::MemoryManager        ascendMemoryManager;
  HiCR::backend::ascend::ComputeManager       ascendComputeManager;
  HiCR::backend::ascend::CommunicationManager ascendCommunicationManager;

  /////////  Allocate input and output buffers on both host and the device
  // First matrix (A)
  auto input1Size   = A * B * sizeof(aclFloat16);
  auto input1Host   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, input1Size);
  auto input1Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input1Size);

  // Second matrix (B)
  auto input2Size   = B * C * sizeof(aclFloat16);
  auto input2Host   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, input2Size);
  auto input2Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input2Size);

  // Third matrix (C)
  auto input3Size   = A * C * sizeof(aclFloat16);
  auto input3Host   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);
  auto input3Device = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input3Size);

  // Alpha coefficient
  auto sizeAlphaBeta = sizeof(aclFloat16);
  auto alphaHost     = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto alphaDevice   = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeAlphaBeta);

  // Beta coefficient
  auto betaHost   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto betaDevice = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeAlphaBeta);

  // Output matrix. Stores (alpha * A * B) + (beta * C)
  auto outputHost   = ascendMemoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);
  auto outputDevice = ascendMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input3Size);

  ///////// Fill matrix with data
  populateMemorySlot(input1Host, A, B, 1.0);
  populateMemorySlot(input2Host, B, C, 1.0);
  populateMemorySlot(input3Host, A, C, 1.0);
  ((aclFloat16 *)alphaHost->getPointer())[0] = aclFloatToFloat16(1.0);
  ((aclFloat16 *)betaHost->getPointer())[0]  = aclFloatToFloat16(1.0);

  ///////// Tensor descriptors definition. Describe the type and shape of data contained in each tensor
  // A matrix
  const int64_t input1Dimensions[]     = {A, B};
  auto          input1TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input1Dimensions, ACL_FORMAT_ND);
  if (input1TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // B matrix
  const int64_t input2Dimensions[]     = {B, C};
  auto          input2TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input2Dimensions, ACL_FORMAT_ND);
  if (input2TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // C and output matrix
  const int64_t input3Dimensions[]     = {A, C};
  auto          input3TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input3Dimensions, ACL_FORMAT_ND);
  if (input3TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Alpha and beta parameters
  const int64_t alphaBetaDimensions[]     = {1};
  auto          alphaBetaTensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 1, alphaBetaDimensions, ACL_FORMAT_ND);
  if (alphaBetaTensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Map the input tensor descriptors with the allocated buffers
  std::vector<ascend::ComputationKernel::tensorData_t> inputs({
    ascend::ComputationKernel::createTensorData(input1Device, input1TensorDescriptor),
    ascend::ComputationKernel::createTensorData(input2Device, input2TensorDescriptor),
    ascend::ComputationKernel::createTensorData(input3Device, input3TensorDescriptor),
    ascend::ComputationKernel::createTensorData(alphaDevice, alphaBetaTensorDescriptor),
    ascend::ComputationKernel::createTensorData(betaDevice, alphaBetaTensorDescriptor),
  });

  // Map the output tensor descriptors with the allocated buffer
  std::vector<ascend::ComputationKernel::tensorData_t> outputs({ascend::ComputationKernel::createTensorData(outputDevice, input3TensorDescriptor)});

  // Create kernel attributes
  auto kernelAttributes = aclopCreateAttr();
  if (kernelAttributes == NULL) HICR_THROW_RUNTIME("Can not create kernel attributes");

  ///////// Kernels definitions
  // Copy the inputs from the host buffers to the device buffers using a MemoryKernel abstraction
  auto copyInput1MemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, input1Device, 0, input1Host, 0, input1Size);
  auto copyInput2MemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, input2Device, 0, input2Host, 0, input2Size);
  auto copyInput3MemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, input3Device, 0, input3Host, 0, input3Size);
  auto copyAlphaMemoryKernel  = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, alphaDevice, 0, alphaHost, 0, sizeAlphaBeta);
  auto copyBetaMemoryKernel   = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, betaDevice, 0, betaHost, 0, sizeAlphaBeta);

  // Copy the result back on the host using a MemoryKernel abstraction
  auto copyOutputMemoryKernel = std::make_shared<ascend::MemoryKernel>(&ascendCommunicationManager, outputHost, 0, outputDevice, 0, input3Size);

  // Create the ComputationKernel by reading it from file
  auto fileComputationKernel =
    createComputeKernelFromFile("/../examples/kernel/op_models/0_GEMM_1_2_128_64_1_2_64_256_1_2_128_256_1_2_1_1_2_1_1_2_128_256.om", inputs, outputs, kernelAttributes);

  // Create the stream of Kernel operations to be executed on the device
  auto operations = std::vector<std::shared_ptr<ascend::Kernel>>{
    copyInput1MemoryKernel, copyInput2MemoryKernel, copyInput3MemoryKernel, copyAlphaMemoryKernel, copyBetaMemoryKernel, fileComputationKernel, copyOutputMemoryKernel};

  // Create execution unit
  auto executionUnit = ascendComputeManager.createExecutionUnit(operations);

  ///////// Execute the kernels through HiCR
  executeKernel(ascendComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("First vector contains: %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input1Host->getPointer())[0]));
  printf("Second vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input2Host->getPointer())[0]));
  printf("Third vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input3Host->getPointer())[0]));
  printf("Vector sum is : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)outputHost->getPointer())[0]));

  // Reset output tensor
  populateMemorySlot(outputHost, A, C, 0.0);

  // Create the ComputationKernel by looking up in a directory
  auto directoryComputationKernel = createComputeKernelFromDirectory("/../examples/kernel/op_models", inputs, outputs, kernelAttributes);

  // Create the stream of Kernel operations to be executed on the device
  operations = std::vector<std::shared_ptr<ascend::Kernel>>{copyInput1MemoryKernel, copyInput2MemoryKernel, directoryComputationKernel, copyOutputMemoryKernel};

  // Create execution unit
  executionUnit = ascendComputeManager.createExecutionUnit(operations);

  ///////// Execute the kernels through HiCR
  executeKernel(ascendComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("First vector contains: %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input1Host->getPointer())[0]));
  printf("Second vector contains : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)input2Host->getPointer())[0]));
  printf("Vector sum is : %.1f\n", aclFloat16ToFloat(((const aclFloat16 *)outputHost->getPointer())[0]));

  // Free memory slots
  ascendMemoryManager.freeLocalMemorySlot(input1Host);
  ascendMemoryManager.freeLocalMemorySlot(input1Device);
  ascendMemoryManager.freeLocalMemorySlot(input2Host);
  ascendMemoryManager.freeLocalMemorySlot(input2Device);
  ascendMemoryManager.freeLocalMemorySlot(input3Host);
  ascendMemoryManager.freeLocalMemorySlot(input3Device);
  ascendMemoryManager.freeLocalMemorySlot(alphaHost);
  ascendMemoryManager.freeLocalMemorySlot(alphaDevice);
  ascendMemoryManager.freeLocalMemorySlot(betaHost);
  ascendMemoryManager.freeLocalMemorySlot(betaDevice);
  ascendMemoryManager.freeLocalMemorySlot(outputHost);
  ascendMemoryManager.freeLocalMemorySlot(outputDevice);

  // Destroy tensor descriptors and kernel attributes
  aclDestroyTensorDesc(input1TensorDescriptor);
  aclDestroyTensorDesc(input2TensorDescriptor);
  aclDestroyTensorDesc(input3TensorDescriptor);
  aclDestroyTensorDesc(alphaBetaTensorDescriptor);
  aclopDestroyAttr(kernelAttributes);

  // Finalize ACL runtime and hwloc
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  hwloc_topology_destroy(topology);

  return 0;
}

std::shared_ptr<ascend::ComputationKernel> createComputeKernelFromFile(const std::string                                    &path,
                                                                       std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                                       std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                                       const aclopAttr                                      *kernelAttributes)
{
  const auto currentPath = std::filesystem::current_path().string();
  auto       kernelPath  = currentPath + std::string(path);

  // Instantiate a ComputationKernel abstraction by providing a path to an .om file. The kernel is loaded internally
  auto kernel = std::make_shared<ascend::ComputationKernel>(kernelPath.c_str(), "GEMM", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}

std::shared_ptr<ascend::ComputationKernel> createComputeKernelFromDirectory(const std::string                                    &path,
                                                                            std::vector<ascend::ComputationKernel::tensorData_t> &inputs,
                                                                            std::vector<ascend::ComputationKernel::tensorData_t> &outputs,
                                                                            const aclopAttr                                      *kernelAttributes)
{
  aclError err;

  const auto currentPath = std::filesystem::current_path().string();
  auto       kernelPath  = currentPath + std::string(path);

  // Set the directory in which ACL will performs the lookup for kernels
  err = aclopSetModelDir(kernelPath.c_str());
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set the model directory %s in ACL runtime. Error: %d", kernelPath.c_str(), err);

  // Instantiate a ComputationKernel abstraction by providing only its features. The kernel has been already loaded in aclopSetModelDir()
  auto kernel = std::make_shared<ascend::ComputationKernel>("GEMM", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}
