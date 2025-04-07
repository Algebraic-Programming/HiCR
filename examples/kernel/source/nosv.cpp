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
#include <nosv.h>
#include <hicr/backends/nosv/common.hpp>
#include <hicr/backends/hwloc/computeResource.hpp>
#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/backends/nosv/computeManager.hpp>
#include <hicr/backends/hwloc/memoryManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "./include/kernel.hpp"

#define A 128
#define B 64
#define C 256

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
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, A, C, B, *alpha, input1, B, input2, C, *beta, input3, C);
}

int main(int argc, char **argv)
{
  // Initialize nosv
  check(nosv_init());

  // nosv task instance for the main thread
  nosv_task_t mainTask;

  // Attaching the main thread
  check(nosv_attach(&mainTask, NULL, NULL, NOSV_ATTACH_NONE));

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc
  // Initializing HWLoc-based host topology manager and retrieve host memory space and compute resource
  HiCR::backend::hwloc::TopologyManager hostTopologyManager(&topology);
  auto                                  hostTopology        = hostTopologyManager.queryTopology();
  auto                                  hostDevice          = *hostTopology.getDevices().begin();
  auto                                  hostMemSpace        = *hostDevice->getMemorySpaceList().begin();
  auto                                  hostComputeResource = *hostDevice->getComputeResourceList().begin();

  // Instantiating hwloc memory manager
  HiCR::backend::hwloc::MemoryManager memoryManager(&topology);

  // Initializing the compute manager
  HiCR::backend::nosv::ComputeManager computeManager;

  /////////  Allocate input and output buffers on both host and the device
  // First matrix (A)
  auto input1Size = A * B * sizeof(double);
  auto input1Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input1Size);

  // Second matrix (B)
  auto input2Size = B * C * sizeof(double);
  auto input2Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input2Size);

  // Third matrix (C)
  auto input3Size = A * C * sizeof(double);
  auto input3Host = memoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);

  // Alpha and beta coefficient
  auto sizeAlphaBeta = sizeof(double);
  auto alphaHost     = memoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);
  auto betaHost      = memoryManager.allocateLocalMemorySlot(hostMemSpace, sizeAlphaBeta);

  // Output matrix. Stores (alpha * A * B) + (beta * C)
  auto outputHost = memoryManager.allocateLocalMemorySlot(hostMemSpace, input3Size);

  ///////// Fill matrix with data
  populateMemorySlot(input1Host, A, B, 1.0);
  populateMemorySlot(input2Host, B, C, 1.0);
  populateMemorySlot(input3Host, A, C, 1.0);
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

  ///////// Execute the kernel through HiCR
  executeKernel(computeManager, hostComputeResource, executionUnit);

  // Print the result
  printf("First vector contains: %.1f\n", ((const double *)input1Host->getPointer())[0]);
  printf("Second vector contains : %.1f\n", ((const double *)input2Host->getPointer())[0]);
  printf("Third vector contains : %.1f\n", ((const double *)input3Host->getPointer())[0]);

  // Free memory slots
  memoryManager.freeLocalMemorySlot(input1Host);
  memoryManager.freeLocalMemorySlot(input2Host);
  memoryManager.freeLocalMemorySlot(input3Host);
  memoryManager.freeLocalMemorySlot(alphaHost);
  memoryManager.freeLocalMemorySlot(betaHost);
  memoryManager.freeLocalMemorySlot(outputHost);

  // Detach main thread
  check(nosv_detach(NOSV_DETACH_NONE));

  // Shutdown nosv
  check(nosv_shutdown());

  // Destroy HWloc topology object
  hwloc_topology_destroy(topology);

  return 0;
}
