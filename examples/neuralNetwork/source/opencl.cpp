#include <CL/opencl.hpp>
#include <cstdio>

#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include <hicr/backends/opencl/computeResource.hpp>
#include <hicr/backends/opencl/memorySpace.hpp>
#include <hicr/backends/opencl/memoryManager.hpp>
#include <hicr/backends/opencl/topologyManager.hpp>
#include <hicr/backends/opencl/communicationManager.hpp>
#include <hicr/backends/opencl/computeManager.hpp>
#include <hicr/core/exceptions.hpp>

#include "./include/network.hpp"
#include "./include/imageLoader.hpp"
#include "./include/factory/executionUnit/opencl/executionUnitFactory.hpp"
#include "./include/tensor/opencl/tensor.hpp"

std::string readFromFile(const std::string &path)
{
  std::string source;
  // get size of file to know how much memory to allocate
  std::uintmax_t filesize = std::filesystem::file_size(path);
  source.resize(filesize);
  std::ifstream fin(path);
  fin.read(source.data(), filesize);
  if (!fin)
  {
    fprintf(stderr, "Error reading file could only read %ld bytes", fin.gcount());
    exit(-1);
  }

  fin.close();

  return source;
}

int main(int argc, char **argv)
{
  ////// Parse arguments
  if (argc < 5) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string onnxModelFilePath = argv[1];
  const std::string imagePathPrefix   = argv[2];
  const std::string labelsFilePath    = argv[3];
  uint64_t          imagesToAnalyze   = std::stoull(argv[4]);
  const std::string kernelsPath       = argv[5];

  ////// Declare backend-specific HiCR resources
  // Creating HWloc topology object
  hwloc_topology_t hwlocTopology;
  hwloc_topology_init(&hwlocTopology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager hostTopologyManager(&hwlocTopology);

  // Initializing opencl topology manager and retrieve memory space and compute resource of one of the devices
  HiCR::backend::opencl::TopologyManager openclTopologyManager;

  // Asking backend to check the available devices
  auto hostTopology   = hostTopologyManager.queryTopology();
  auto deviceTopology = openclTopologyManager.queryTopology();

  // Getting first device found in the topology
  auto host         = *hostTopology.getDevices().begin();
  auto device       = *deviceTopology.getDevices().begin();
  auto openclDevice = dynamic_pointer_cast<HiCR::backend::opencl::Device>(device);

  // Getting memory spaces and pick the first one found
  auto hostMemorySpaces       = host->getMemorySpaceList();
  auto deviceMemorySpaces     = device->getMemorySpaceList();
  auto deviceComputeResources = device->getComputeResourceList();
  auto hostMemorySpace        = *hostMemorySpaces.begin();
  auto deviceMemorySpace      = *deviceMemorySpaces.begin();
  auto deviceComputeResource  = *deviceComputeResources.begin();

  // Declare OpenCL context
  auto devices        = {openclDevice->getOpenCLDevice()};
  auto defaultContext = std::make_shared<cl::Context>(devices);

  // Create command queues for each device
  std::unordered_map<HiCR::backend::opencl::Device::deviceIdentifier_t, std::shared_ptr<cl::CommandQueue>> deviceQueueMap;
  deviceQueueMap[openclDevice->getId()] = std::make_shared<cl::CommandQueue>(*defaultContext, openclDevice->getOpenCLDevice());

  // Declare Opencl memory, communication, and compute managers
  HiCR::backend::opencl::MemoryManager        openclMemoryManager(deviceQueueMap);
  HiCR::backend::opencl::CommunicationManager openclCommunicationManager(deviceQueueMap);
  HiCR::backend::opencl::ComputeManager       openclComputeManager(defaultContext);

  auto deviceProcessingUnit = openclComputeManager.createProcessingUnit(deviceComputeResource);

  // Build OpenCL program
  auto                 source = readFromFile(kernelsPath);
  cl::Program::Sources sources;
  sources.push_back({source.c_str(), source.size()});
  cl::Program program(*defaultContext, sources);
  auto        err = program.build({openclDevice->getOpenCLDevice()});
  if (err != CL_SUCCESS)
  {
    auto log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(openclDevice->getOpenCLDevice());
    HICR_THROW_RUNTIME("Can not build program. Error:\n%s\n", log.c_str());
  }

  // Create execution unit factory
  auto executioUnitFactory =
    factory::opencl::ExecutionUnitFactory(openclComputeManager, openclCommunicationManager, openclMemoryManager, deviceMemorySpace, hostMemorySpace, program);

  ////// Load ONNX model
  // Declare ONNX model
  onnx::ModelProto model;

  // Create stream to read the model from file
  std::ifstream modelFile(onnxModelFilePath, std::ios::binary);

  // Read the model
  if (!model.ParseFromIstream(&modelFile)) { HICR_THROW_RUNTIME("Failed to parse the model."); }

  // Load MNIST labels
  auto labels     = loadLabels(labelsFilePath);
  imagesToAnalyze = std::min(imagesToAnalyze, labels.size());

  uint64_t failures      = 0;

  for (uint64_t i = 0; i < imagesToAnalyze; i++)
  {
    // Create the neural network
    auto neuralNetwork = NeuralNetwork(openclComputeManager,
                                       std::move(deviceProcessingUnit),
                                       openclCommunicationManager,
                                       openclMemoryManager,
                                       deviceMemorySpace,
                                       executioUnitFactory,
                                       tensor::opencl::Tensor::create,
                                       tensor::opencl::Tensor::clone);

    // Load data of the pre-trained model
    neuralNetwork.loadPreTrainedData(model, hostMemorySpace);

    // Create the imageTensor
    auto imageFilePath = imagePathPrefix + "/image_" + std::to_string(i) + ".bin";
    auto imageTensor   = loadImage(imageFilePath, openclCommunicationManager, openclMemoryManager, hostMemorySpace, hostMemorySpace, tensor::opencl::Tensor::create);

    // Run the inference on the imageTensor
    const auto output = neuralNetwork.forward(imageTensor);

    deviceProcessingUnit = neuralNetwork.releaseProcessingUnit();

    auto o = dynamic_pointer_cast<tensor::opencl::Tensor>(output);
    if (o == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to supported type"); }

    auto hostOutputTensor = openclMemoryManager.allocateLocalMemorySlot(hostMemorySpace, o->getData()->getSize());
    openclCommunicationManager.memcpy(hostOutputTensor, 0, o->getData(), 0, o->getData()->getSize());

    auto desiredPrediction = labels[i];
    auto actualPrediction  = neuralNetwork.getPrediction(output->getData(), output->size());

    if (desiredPrediction != actualPrediction) { failures++; }

    openclMemoryManager.freeLocalMemorySlot(hostOutputTensor);

    // Free the input image tensor
    openclMemoryManager.freeLocalMemorySlot(imageTensor->getData());
    if (i % 100 == 0 && i > 0) { printf("Analyzed images: %lu/%lu\n", i, labels.size()); }
  }

  printf("Total failures: %lu/%lu\n", failures, imagesToAnalyze);
  //Destroy hwloc topology object
  hwloc_topology_destroy(hwlocTopology);

  return 0;
}
