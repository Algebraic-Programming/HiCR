#include <fstream>
#include <memory>
#include <iomanip>
#include <stdio.h>
#include <CL/opencl.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/opencl/computationKernel.hpp>
#include <hicr/backends/opencl/kernel.hpp>
#include <hicr/backends/opencl/memoryKernel.hpp>
#include <hicr/backends/opencl/L0/executionUnit.hpp>
#include <hicr/backends/opencl/L0/processingUnit.hpp>
#include <hicr/backends/opencl/L1/memoryManager.hpp>
#include <hicr/backends/opencl/L1/topologyManager.hpp>
#include <hicr/backends/opencl/L1/communicationManager.hpp>
#include <hicr/backends/opencl/L1/computeManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>

#include "./include/kernel.hpp"

#define M 16
#define N 16
#define K 16

namespace opencl = HiCR::backend::opencl;

std::string readFromFile(const std::string &path)
{
  std::string source;
  // get size of file to know how much memory to allocate
  std::uintmax_t filesize = std::filesystem::file_size(path);
  source.resize(filesize);
  std::ifstream fin(path);
  fin.read(source.data(), filesize);
  if (!fin) { HICR_THROW_RUNTIME("Error reading file could only read %ld bytes", fin.gcount()); }

  fin.close();

  return source;
}

/**
 * Populate a matrix contained in a memory slot with the desired value converted to float
 * 
 * @param[inout] memorySlot memory slot containing the matrix
 * @param[in] rows 
 * @param[in] columns
 * @param[in] value 
*/
void populateMemorySlot(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, int rows, int columns, float value)
{
  for (int i = 0; i < rows * columns; i++) { ((float *)memorySlot->getPointer())[i] = value; }
}

int main(int argc, char **argv)
{
  if (argc < 2) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string kernelPath = argv[1];

  // Creating HWloc topology object
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);

  ///////// Instantiate HiCR-specific entities for hwloc and opencl
  // Initializing HWLoc-based host topology manager and retrieve host memory space
  HiCR::backend::hwloc::L1::TopologyManager hostTopologyManager(&topology);
  auto                                      hostTopology = hostTopologyManager.queryTopology();
  auto                                      hostDevice   = *hostTopology.getDevices().begin();
  auto                                      hostMemSpace = *hostDevice->getMemorySpaceList().begin();

  // Initializing opencl topology manager and retrieve memory space and compute resource of one of the devices
  HiCR::backend::opencl::L1::TopologyManager openclTopologyManager;
  auto                                       openclTopology = openclTopologyManager.queryTopology();
  if (openclTopology.getDevices().empty()) { HICR_THROW_RUNTIME("No devices detected"); }
  auto openclDevice          = *openclTopology.getDevices().begin();
  auto clDevice              = dynamic_pointer_cast<HiCR::backend::opencl::L0::Device>(openclDevice);
  auto deviceMemSpace        = *openclDevice->getMemorySpaceList().begin();
  auto deviceComputeResource = *openclDevice->getComputeResourceList().begin();

  auto devices         = std::vector<cl::Device>({clDevice->getOpenCLDevice()});
  auto defatultContext = std::make_shared<cl::Context>(devices);

  std::unordered_map<HiCR::backend::opencl::L0::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> deviceQueueMap;
  deviceQueueMap[clDevice->getId()] = std::make_shared<cl::CommandQueue>(*defatultContext, clDevice->getOpenCLDevice());

  // Instantiating OpenCL memory, compute, and communication manager
  HiCR::backend::opencl::L1::MemoryManager        openclMemoryManager(deviceQueueMap);
  HiCR::backend::opencl::L1::ComputeManager       openclComputeManager(defatultContext);
  HiCR::backend::opencl::L1::CommunicationManager openclCommunicationManager(deviceQueueMap);

  // Build OpenCL program
  auto                 source = readFromFile(kernelPath);
  cl::Program::Sources sources;
  sources.push_back({source.c_str(), source.size()});
  cl::Program program(*defatultContext, sources);
  auto        err = program.build({clDevice->getOpenCLDevice()});
  if (err != CL_SUCCESS) { HICR_THROW_RUNTIME("Can not build program. Error: %d\n", err); }

  /////////  Allocate input and output buffers on both host and the device
  // First matrix (M)
  auto sizeA = M * N * sizeof(float);
  auto A_h   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeA);
  auto A_d   = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeA);

  // Second matrix (N)
  auto sizeB = N * K * sizeof(float);
  auto B_h   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeB);
  auto B_d   = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeB);

  // Third matrix (K)
  auto sizeC = M * K * sizeof(float);
  auto C_h   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeC);
  auto C_d   = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeC);

  // Alpha coefficient
  auto sizeMNK = sizeof(unsigned int);
  auto M_h     = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeMNK);
  auto M_d     = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeMNK);

  // Beta coefficient
  auto N_h = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeMNK);
  auto N_d = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeMNK);

  // Beta coefficient
  auto K_h = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeMNK);
  auto K_d = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeMNK);

  // Output matrix. Stores (alpha * M * N) + (beta * K)
  auto outputHost = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeC);

  ///////// Fill matrix with data
  populateMemorySlot(A_h, M, N, 1.0);
  populateMemorySlot(B_h, N, K, 1.0);
  populateMemorySlot(C_h, M, K, 1.0);
  *(unsigned int *)M_h->getPointer() = M;
  *(unsigned int *)N_h->getPointer() = N;
  *(unsigned int *)K_h->getPointer() = K;

  // Map the input tensor descriptors with the allocated buffers
  std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> kernelArgs({
    M_d,
    N_d,
    K_d,
    A_d,
    B_d,
    C_d,
  });

  ///////// Kernels definitions
  // Copy the kernelArgs from the host buffers to the device buffers using a MemoryKernel abstraction
  auto copyAMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, A_d, 0, A_h, 0, sizeA);
  auto copyBMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, B_d, 0, B_h, 0, sizeB);
  auto copyCMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, C_d, 0, C_h, 0, sizeC);
  auto copyMMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, M_d, 0, M_h, 0, sizeMNK);
  auto copyNMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, N_d, 0, N_h, 0, sizeMNK);
  auto copyKMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, K_d, 0, K_h, 0, sizeMNK);

  // Copy the result back on the host using a MemoryKernel abstraction
  auto copyOutputMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, outputHost, 0, C_d, 0, sizeC);

  // Create the ComputationKernel by reading it from file
  // Create kernel object
  auto kernel = std::make_shared<cl::Kernel>(program, "gemm_kernel");

  // Define global and local work sizes
  cl::NDRange global(M, K); // Global work size: (M x N)
  auto        GEMMKernel = std::make_shared<opencl::ComputationKernel>(kernel, std::move(kernelArgs), cl::NullRange, global, cl::NullRange);

  // Create the stream of Kernel operations to be executed on the device
  auto operations = std::vector<std::shared_ptr<opencl::Kernel>>{
    copyAMemoryKernel, copyBMemoryKernel, copyCMemoryKernel, copyMMemoryKernel, copyNMemoryKernel, copyKMemoryKernel, GEMMKernel, copyOutputMemoryKernel};

  printf("Create execution unit\n");
  // Create execution unit
  auto executionUnit = openclComputeManager.createExecutionUnit(operations);

  printf("Execute kernel\n");
  ///////// Execute the kernels through HiCR
  executeKernel(openclComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("First vector contains: %.1f\n", ((const float *)A_h->getPointer())[0]);
  printf("Second vector contains : %.1f\n", ((const float *)B_h->getPointer())[0]);
  printf("Third vector contains : %.1f\n", ((const float *)C_h->getPointer())[0]);
  printf("Vector sum is : %.1f\n", ((const float *)outputHost->getPointer())[0]);

  // Free memory slots
  openclMemoryManager.freeLocalMemorySlot(A_h);
  openclMemoryManager.freeLocalMemorySlot(A_d);
  openclMemoryManager.freeLocalMemorySlot(B_h);
  openclMemoryManager.freeLocalMemorySlot(B_d);
  openclMemoryManager.freeLocalMemorySlot(C_h);
  openclMemoryManager.freeLocalMemorySlot(C_d);
  openclMemoryManager.freeLocalMemorySlot(M_h);
  openclMemoryManager.freeLocalMemorySlot(M_d);
  openclMemoryManager.freeLocalMemorySlot(N_h);
  openclMemoryManager.freeLocalMemorySlot(N_d);
  openclMemoryManager.freeLocalMemorySlot(outputHost);

  hwloc_topology_destroy(topology);

  return 0;
}
