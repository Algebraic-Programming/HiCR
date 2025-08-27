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
#include <hicr/backends/acl/computationKernel.hpp>
#include <hicr/backends/acl/kernel.hpp>
#include <hicr/backends/acl/memoryKernel.hpp>
#include <hicr/backends/acl/executionUnit.hpp>
#include <hicr/backends/acl/processingUnit.hpp>
#include <hicr/backends/acl/memoryManager.hpp>
#include <hicr/backends/acl/topologyManager.hpp>
#include <hicr/backends/acl/communicationManager.hpp>
#include <hicr/backends/acl/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "./include/common.hpp"
#include "./include/kernel.hpp"

namespace acl = HiCR::backend::acl;

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
 * Print the matrix contained in a local memory slot
 * 
 * \param[in] memSlot memory slot containing the matrix
 * \param[in] rows matrix rows
 * \param[in] columns matrix columns
*/
void printMatrix(const std::shared_ptr<HiCR::LocalMemorySlot> &memSlot, uint32_t rows, uint32_t columns)
{
  for (uint32_t i = 0; i < rows; i++)
  {
    for (uint32_t j = 0; j < columns; j++) { printf("%.1f ", aclFloat16ToFloat(((const aclFloat16 *)memSlot->getPointer())[i * columns + j])); }
    printf("\n");
  }
}

/**
 * Create a Compute Kernel from a single .om file
 * 
 * @param[in] path the absolute path to the .om file
 * @param[in] inputs input tensor related data structure
 * @param[in] outputs output tensor related data structure
 * @param[in] kernelAttributes kernel attributes
 * 
 * @return the acl compute kernel
*/
std::shared_ptr<acl::ComputationKernel> createComputeKernelFromFile(const std::string                                    &path,
                                                                       std::vector<acl::ComputationKernel::tensorData_t> &inputs,
                                                                       std::vector<acl::ComputationKernel::tensorData_t> &outputs,
                                                                       const aclopAttr                                      *kernelAttributes);

/**
 * Create a Compute Kernel from a single .om file
 * 
 * @param[in] path the absolute path to the folder containing the applications .om files
 * @param[in] inputs input tensor related data structure
 * @param[in] outputs output tensor related data structure
 * @param[in] kernelAttributes kernel attributes
 * 
 * @return the acl compute kernel
*/
std::shared_ptr<acl::ComputationKernel> createComputeKernelFromDirectory(const std::string                                    &path,
                                                                            std::vector<acl::ComputationKernel::tensorData_t> &inputs,
                                                                            std::vector<acl::ComputationKernel::tensorData_t> &outputs,
                                                                            const aclopAttr                                      *kernelAttributes);

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize acl. Error %d", err);

  // Creating HWloc topology object
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc and acl
  // Initializing HWLoc-based host topology manager and retrieve host memory space
  HiCR::backend::hwloc::TopologyManager hostTopologyManager(&topology);
  auto                                  hostTopology = hostTopologyManager.queryTopology();
  auto                                  hostDevice   = *hostTopology.getDevices().begin();
  auto                                  hostMemSpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing acl topology manager and retrieve memory space and compute resource of one of the devices
  HiCR::backend::acl::TopologyManager aclTopologyManager;
  auto                                   aclTopology        = aclTopologyManager.queryTopology();
  auto                                   aclDevice          = *aclTopology.getDevices().begin();
  auto                                   deviceMemSpace        = *aclDevice->getMemorySpaceList().begin();
  auto                                   deviceComputeResource = *aclDevice->getComputeResourceList().begin();

  // Instantiating acl memory, compute, and communication manager
  HiCR::backend::acl::MemoryManager        aclMemoryManager;
  HiCR::backend::acl::ComputeManager       aclComputeManager;
  HiCR::backend::acl::CommunicationManager aclCommunicationManager;

  /////////  Allocate input and output buffers on both host and the device
  // First matrix [M, K]
  auto input1Size   = M * K * sizeof(aclFloat16);
  auto input1Host   = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, input1Size);
  auto input1Device = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input1Size);

  // Second matrix [K, N]
  auto input2Size   = K * N * sizeof(aclFloat16);
  auto input2Host   = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, input2Size);
  auto input2Device = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input2Size);

  // Third matrix [M, N]
  auto input3Size   = M * N * sizeof(aclFloat16);
  auto input3Host   = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);
  auto input3Device = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input3Size);

  // Alpha coefficient
  auto sizeAlphaBeta = sizeof(aclFloat16);
  auto alphaHost     = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto alphaDevice   = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeAlphaBeta);

  // Beta coefficient
  auto betaHost   = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto betaDevice = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeAlphaBeta);

  // Output matrix. Stores (alpha * M * N) + (beta * K)
  auto outputHost   = aclMemoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);
  auto outputDevice = aclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, input3Size);

  ///////// Fill matrix with data
  populateMemorySlot(input1Host, M, K, 1.0);
  populateMemorySlot(input2Host, K, N, 1.0);
  populateMemorySlot(input3Host, M, N, 1.0);
  ((aclFloat16 *)alphaHost->getPointer())[0] = aclFloatToFloat16(1.0);
  ((aclFloat16 *)betaHost->getPointer())[0]  = aclFloatToFloat16(1.0);

  ///////// Tensor descriptors definition. Describe the type and shape of data contained in each tensor
  // M matrix
  const int64_t input1Dimensions[]     = {M, K};
  auto          input1TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input1Dimensions, ACL_FORMAT_ND);
  if (input1TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // N matrix
  const int64_t input2Dimensions[]     = {K, N};
  auto          input2TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input2Dimensions, ACL_FORMAT_ND);
  if (input2TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // K and output matrix
  const int64_t input3Dimensions[]     = {M, N};
  auto          input3TensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 2, input3Dimensions, ACL_FORMAT_ND);
  if (input3TensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Alpha and beta parameters
  const int64_t alphaBetaDimensions[]     = {1};
  auto          alphaBetaTensorDescriptor = aclCreateTensorDesc(ACL_FLOAT16, 1, alphaBetaDimensions, ACL_FORMAT_ND);
  if (alphaBetaTensorDescriptor == NULL) HICR_THROW_RUNTIME("Can not create tensor descriptor");

  // Map the input tensor descriptors with the allocated buffers
  std::vector<acl::ComputationKernel::tensorData_t> inputs({
    acl::ComputationKernel::createTensorData(input1Device, input1TensorDescriptor),
    acl::ComputationKernel::createTensorData(input2Device, input2TensorDescriptor),
    acl::ComputationKernel::createTensorData(input3Device, input3TensorDescriptor),
    acl::ComputationKernel::createTensorData(alphaDevice, alphaBetaTensorDescriptor),
    acl::ComputationKernel::createTensorData(betaDevice, alphaBetaTensorDescriptor),
  });

  // Map the output tensor descriptors with the allocated buffer
  std::vector<acl::ComputationKernel::tensorData_t> outputs({acl::ComputationKernel::createTensorData(outputDevice, input3TensorDescriptor)});

  // Create kernel attributes
  auto kernelAttributes = aclopCreateAttr();
  if (kernelAttributes == NULL) HICR_THROW_RUNTIME("Can not create kernel attributes");

  ///////// Kernels definitions
  // Copy the inputs from the host buffers to the device buffers using a MemoryKernel abstraction
  auto copyInput1MemoryKernel = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, input1Device, 0, input1Host, 0, input1Size);
  auto copyInput2MemoryKernel = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, input2Device, 0, input2Host, 0, input2Size);
  auto copyInput3MemoryKernel = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, input3Device, 0, input3Host, 0, input3Size);
  auto copyAlphaMemoryKernel  = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, alphaDevice, 0, alphaHost, 0, sizeAlphaBeta);
  auto copyBetaMemoryKernel   = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, betaDevice, 0, betaHost, 0, sizeAlphaBeta);

  // Copy the result back on the host using a MemoryKernel abstraction
  auto copyOutputMemoryKernel = std::make_shared<acl::MemoryKernel>(&aclCommunicationManager, outputHost, 0, outputDevice, 0, input3Size);

  // Create the ComputationKernel by reading it from file
  auto fileComputationKernel =
    createComputeKernelFromFile("/../examples/kernel/op_models/0_GEMM_1_2_4_8_1_2_8_2_1_2_4_2_1_2_1_1_2_1_1_2_4_2.om", inputs, outputs, kernelAttributes);

  // Create the stream of Kernel operations to be executed on the device
  auto operations = std::vector<std::shared_ptr<acl::Kernel>>{
    copyInput1MemoryKernel, copyInput2MemoryKernel, copyInput3MemoryKernel, copyAlphaMemoryKernel, copyBetaMemoryKernel, fileComputationKernel, copyOutputMemoryKernel};

  // Create execution unit
  auto executionUnit = aclComputeManager.createExecutionUnit(operations);

  // Print input matrices
  printf("First matrix [M, K]\n");
  printMatrix(input1Host, M, K);
  printf("\nSecond matrix [K, N]\n");
  printMatrix(input2Host, K, N);
  printf("\nThird matrix [M, N]\n");
  printMatrix(input3Host, M, N);

  ///////// Execute the kernels through HiCR
  executeKernel(aclComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("\nOutput matrix [M, N]\n");
  printMatrix(outputHost, M, N);

  // Reset output tensor
  populateMemorySlot(outputHost, M, N, 0.0);

  // Create the ComputationKernel by looking up in a directory
  auto directoryComputationKernel = createComputeKernelFromDirectory("/../examples/kernel/op_models", inputs, outputs, kernelAttributes);

  // Create the stream of Kernel operations to be executed on the device
  operations = std::vector<std::shared_ptr<acl::Kernel>>{copyInput1MemoryKernel, copyInput2MemoryKernel, directoryComputationKernel, copyOutputMemoryKernel};

  // Create execution unit
  executionUnit = aclComputeManager.createExecutionUnit(operations);

  ///////// Execute the kernels through HiCR
  executeKernel(aclComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("\nOutput matrix [M, N]\n");
  printMatrix(outputHost, M, N);

  // Free memory slots
  aclMemoryManager.freeLocalMemorySlot(input1Host);
  aclMemoryManager.freeLocalMemorySlot(input1Device);
  aclMemoryManager.freeLocalMemorySlot(input2Host);
  aclMemoryManager.freeLocalMemorySlot(input2Device);
  aclMemoryManager.freeLocalMemorySlot(input3Host);
  aclMemoryManager.freeLocalMemorySlot(input3Device);
  aclMemoryManager.freeLocalMemorySlot(alphaHost);
  aclMemoryManager.freeLocalMemorySlot(alphaDevice);
  aclMemoryManager.freeLocalMemorySlot(betaHost);
  aclMemoryManager.freeLocalMemorySlot(betaDevice);
  aclMemoryManager.freeLocalMemorySlot(outputHost);
  aclMemoryManager.freeLocalMemorySlot(outputDevice);

  // Destroy tensor descriptors and kernel attributes
  aclDestroyTensorDesc(input1TensorDescriptor);
  aclDestroyTensorDesc(input2TensorDescriptor);
  aclDestroyTensorDesc(input3TensorDescriptor);
  aclDestroyTensorDesc(alphaBetaTensorDescriptor);
  aclopDestroyAttr(kernelAttributes);

  // Finalize ACL runtime and hwloc
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize acl. Error %d", err);

  hwloc_topology_destroy(topology);

  return 0;
}

std::shared_ptr<acl::ComputationKernel> createComputeKernelFromFile(const std::string                                    &path,
                                                                       std::vector<acl::ComputationKernel::tensorData_t> &inputs,
                                                                       std::vector<acl::ComputationKernel::tensorData_t> &outputs,
                                                                       const aclopAttr                                      *kernelAttributes)
{
  const auto currentPath = std::filesystem::current_path().string();
  auto       kernelPath  = currentPath + std::string(path);

  // Instantiate a ComputationKernel abstraction by providing a path to an .om file. The kernel is loaded internally
  auto kernel = std::make_shared<acl::ComputationKernel>(kernelPath.c_str(), "GEMM", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}

std::shared_ptr<acl::ComputationKernel> createComputeKernelFromDirectory(const std::string                                    &path,
                                                                            std::vector<acl::ComputationKernel::tensorData_t> &inputs,
                                                                            std::vector<acl::ComputationKernel::tensorData_t> &outputs,
                                                                            const aclopAttr                                      *kernelAttributes)
{
  aclError err;

  const auto currentPath = std::filesystem::current_path().string();
  auto       kernelPath  = currentPath + std::string(path);

  // Set the directory in which ACL will performs the lookup for kernels
  err = aclopSetModelDir(kernelPath.c_str());
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set the model directory %s in ACL runtime. Error: %d", kernelPath.c_str(), err);

  // Instantiate a ComputationKernel abstraction by providing only its features. The kernel has been already loaded in aclopSetModelDir()
  auto kernel = std::make_shared<acl::ComputationKernel>("GEMM", std::move(inputs), std::move(outputs), kernelAttributes);
  return kernel;
}
