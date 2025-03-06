#include <cblas.h>
#include <filesystem>
#include <memory>
#include <stdio.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/pthreads/L1/communicationManager.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/hwloc/L1/memoryManager.hpp>

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
void populateMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, int rows, int columns, double value)
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
  // Creating HWloc topology object
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc and pthreads
  // Initializing HWLoc-based host topology manager and retrieve host memory space and compute resource
  HiCR::backend::hwloc::L1::TopologyManager hostTopologyManager(&topology);
  auto                                      hostTopology        = hostTopologyManager.queryTopology();
  auto                                      hostDevice          = *hostTopology.getDevices().begin();
  auto                                      hostMemSpace        = *hostDevice->getMemorySpaceList().begin();
  auto                                      hostComputeResource = *hostDevice->getComputeResourceList().begin();

  // Instantiating hwloc memory manager
  HiCR::backend::hwloc::L1::MemoryManager memoryManager(&topology);

  // Instantiating pthreads compute manager
  HiCR::backend::pthreads::L1::ComputeManager computeManager;

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

  // Finalize hwloc
  hwloc_topology_destroy(topology);
  return 0;
}
