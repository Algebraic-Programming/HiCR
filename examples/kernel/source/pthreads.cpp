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

#include <cblas.h>
#include <filesystem>
#include <memory>
#include <stdio.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/hwloc/memoryManager.hpp>

#include "./include/common.hpp"
#include "./include/kernel.hpp"

/**
 * Populate a matrix contained in a memory slot with the desired value converted to aclFloat16
 * 
 * @param[inout] memorySlot memory slot containing the matrix
 * @param[in] rows 
 * @param[in] columns
 * @param[in] value 
*/
void populateMemorySlot(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot, int rows, int columns, double value)
{
  for (int i = 0; i < rows * columns; i++) { ((double *)memorySlot->getPointer())[i] = value; }
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
    for (uint32_t j = 0; j < columns; j++) { printf("%.1f ", ((const double *)memSlot->getPointer())[i * columns + j]); }
    printf("\n");
  }
}

/**
 * Wrapper for cblas_dgemm operation
 * 
 * @param[in] input1 first matrix
 * @param[in] input2 second matrix
 * @param[inout] input3 third matrix. Used as accumulator
 * @param[in] alpha
 * @param[in] beta 
*/
__INLINE__ void gemm(double *input1, double *input2, double *input3, double *alpha, double *beta)
{
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, *alpha, input1, K, input2, N, *beta, input3, N);
}

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc and pthreads
  // Initializing HWLoc-based host topology manager and retrieve host memory space and compute resource
  HiCR::backend::hwloc::TopologyManager hostTopologyManager(&topology);
  auto                                      hostTopology        = hostTopologyManager.queryTopology();
  auto                                      hostDevice          = *hostTopology.getDevices().begin();
  auto                                      hostMemSpace        = *hostDevice->getMemorySpaceList().begin();
  auto                                      hostComputeResource = *hostDevice->getComputeResourceList().begin();

  // Instantiating hwloc memory manager
  HiCR::backend::hwloc::MemoryManager memoryManager(&topology);

  // Instantiating pthreads compute manager
  HiCR::backend::pthreads::ComputeManager computeManager;

  /////////  Allocate input and output buffers on both host and the device
  // First matrix [M, K]
  auto input1Size = M * K * sizeof(double);
  auto input1Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input1Size);

  // Second matrix [K, N]
  auto input2Size = K * N * sizeof(double);
  auto input2Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input2Size);

  // Third matrix [M, N]
  auto input3Size = M * N * sizeof(double);
  auto input3Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);

  // Alpha and beta coefficient
  auto sizeAlphaBeta = sizeof(double);
  auto alphaHost     = memoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto betaHost      = memoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);

  // Output matrix. Stores (alpha * M * N) + (beta * K)
  auto outputHost = memoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);

  ///////// Fill matrix with data
  populateMemorySlot(input1Host, M, K, 1.0);
  populateMemorySlot(input2Host, K, N, 1.0);
  populateMemorySlot(input3Host, M, N, 1.0);
  ((double *)alphaHost->getPointer())[0] = 1.0;
  ((double *)betaHost->getPointer())[0]  = 1.0;

  // Create execution unit
  auto executionUnit = computeManager.createExecutionUnit([=](void *arg) {
    gemm((double *)input1Host->getPointer(),
         (double *)input2Host->getPointer(),
         (double *)input3Host->getPointer(),
         (double *)alphaHost->getPointer(),
         (double *)betaHost->getPointer());
  });

  // Print input matrices
  printf("First matrix [M, K]\n");
  printMatrix(input1Host, M, K);
  printf("\nSecond matrix [K, N]\n");
  printMatrix(input2Host, K, N);
  printf("\nThird matrix [M, N]\n");
  printMatrix(input3Host, M, N);

  ///////// Execute the kernel through HiCR
  executeKernel(computeManager, hostComputeResource, executionUnit);

  // Print the result
  printf("\nOutput matrix [M, N]\n");
  printMatrix(input3Host, M, N);

  // Free memory slots
  memoryManager.freeLocalMemorySlot(input1Host);
  memoryManager.freeLocalMemorySlot(input2Host);
  memoryManager.freeLocalMemorySlot(input3Host);
  memoryManager.freeLocalMemorySlot(alphaHost);
  memoryManager.freeLocalMemorySlot(betaHost);
  memoryManager.freeLocalMemorySlot(outputHost);

  // Finalize hwloc
  hwloc_topology_destroy(topology);
  return 0;
}
