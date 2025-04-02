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

#include "./include/common.hpp"
#include "./include/kernel.hpp"

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

/**
 * Print the matrix contained in a local memory slot
 * 
 * \param[in] memSlot memory slot containing the matrix
 * \param[in] rows matrix rows
 * \param[in] columns matrix columns
*/
void printMatrix(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memSlot, uint32_t rows, uint32_t columns)
{
  for (uint32_t i = 0; i < rows; i++)
  {
    for (uint32_t j = 0; j < columns; j++) { printf("%.1f ", ((const float *)memSlot->getPointer())[i * columns + j]); }
    printf("\n");
  }
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
  // First matrix [M, K]
  auto sizeInput1   = M * K * sizeof(float);
  auto input1Host   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeInput1);
  auto input1Device = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeInput1);

  // Second matrix [K, N]
  auto sizeInput2   = K * N * sizeof(float);
  auto input2Host   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeInput2);
  auto input2Device = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeInput2);

  // Third matrix [M, N]
  auto sizeInput3   = M * N * sizeof(float);
  auto input3Host   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeInput3);
  auto input3Device = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeInput3);

  // input1 matrix dimension
  auto sizeABC = sizeof(unsigned int);
  auto MHost   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeABC);
  auto ADevice = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeABC);

  // input2 matrix dimension
  auto NHost   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeABC);
  auto BDevice = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeABC);

  // input3 matrix dimension
  auto KHost   = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeABC);
  auto CDevice = openclMemoryManager.allocateLocalMemorySlot(deviceMemSpace, sizeABC);

  // Output matrix. Stores (alpha * input1 * input2) + (beta * input3)
  auto outputHost = openclMemoryManager.allocateLocalMemorySlot(hostMemSpace, sizeInput3);

  ///////// Fill matrix with data
  populateMemorySlot(input1Host, M, K, 1.0);
  populateMemorySlot(input2Host, K, N, 1.0);
  populateMemorySlot(input3Host, M, N, 1.0);
  *(unsigned int *)MHost->getPointer() = M;
  *(unsigned int *)NHost->getPointer() = N;
  *(unsigned int *)KHost->getPointer() = K;

  // Print input matrices
  printf("First matrix [M, K]\n");
  printMatrix(input1Host, M, K);
  printf("\nSecond matrix [K, N]\n");
  printMatrix(input2Host, K, N);
  printf("\nThird matrix [M, N]\n");
  printMatrix(input3Host, M, N);

  // Map the input tensor descriptors with the allocated buffers
  std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> kernelArgs({ADevice, BDevice, CDevice, input1Device, input2Device, input3Device});

  ///////// Kernels definitions
  // Copy the kernelArgs from the host buffers to the device buffers using a MemoryKernel abstraction
  auto copyAMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, input1Device, 0, input1Host, 0, sizeInput1);
  auto copyBMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, input2Device, 0, input2Host, 0, sizeInput2);
  auto copyCMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, input3Device, 0, input3Host, 0, sizeInput3);
  auto copyMMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, ADevice, 0, MHost, 0, sizeABC);
  auto copyNMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, BDevice, 0, NHost, 0, sizeABC);
  auto copyKMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, CDevice, 0, KHost, 0, sizeABC);

  // Copy the result back on the host using a MemoryKernel abstraction
  auto copyOutputMemoryKernel = std::make_shared<opencl::MemoryKernel>(&openclCommunicationManager, outputHost, 0, input3Device, 0, sizeInput3);

  // Create the ComputationKernel by reading it from file
  // Create kernel object
  auto kernel = std::make_shared<cl::Kernel>(program, "gemm_kernel");

  // Define global and local work sizes
  auto global = cl::NDRange(M, N);
  auto GEMMKernel = std::make_shared<opencl::ComputationKernel>(kernel, std::move(kernelArgs), cl::NullRange, global, cl::NullRange);

  // Create the stream of Kernel operations to be executed on the device
  auto operations = std::vector<std::shared_ptr<opencl::Kernel>>{
    copyAMemoryKernel, copyBMemoryKernel, copyCMemoryKernel, copyMMemoryKernel, copyNMemoryKernel, copyKMemoryKernel, GEMMKernel, copyOutputMemoryKernel};

  // Create execution unit
  auto executionUnit = openclComputeManager.createExecutionUnit(operations);

  ///////// Execute the kernels through HiCR
  executeKernel(openclComputeManager, deviceComputeResource, executionUnit);

  // Print the result
  printf("\nOutput matrix [M, N]\n");
  printMatrix(outputHost, M, N);

  // Free memory slots
  openclMemoryManager.freeLocalMemorySlot(input1Host);
  openclMemoryManager.freeLocalMemorySlot(input1Device);
  openclMemoryManager.freeLocalMemorySlot(input2Host);
  openclMemoryManager.freeLocalMemorySlot(input2Device);
  openclMemoryManager.freeLocalMemorySlot(input3Host);
  openclMemoryManager.freeLocalMemorySlot(input3Device);
  openclMemoryManager.freeLocalMemorySlot(MHost);
  openclMemoryManager.freeLocalMemorySlot(ADevice);
  openclMemoryManager.freeLocalMemorySlot(NHost);
  openclMemoryManager.freeLocalMemorySlot(BDevice);
  openclMemoryManager.freeLocalMemorySlot(outputHost);

  hwloc_topology_destroy(topology);

  return 0;
}
