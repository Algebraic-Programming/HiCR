#include <acl/acl.h>
#include <cstdio>

#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include <hicr/backends/acl/computeResource.hpp>
#include <hicr/backends/acl/memorySpace.hpp>
#include <hicr/backends/acl/memoryManager.hpp>
#include <hicr/backends/acl/topologyManager.hpp>
#include <hicr/backends/acl/communicationManager.hpp>
#include <hicr/backends/acl/computeManager.hpp>
#include <hicr/core/exceptions.hpp>

#include "./include/network.hpp"
#include "./include/imageLoader.hpp"
#include "./include/factory/executionUnit/acl/executionUnitFactory.hpp"
#include "./include/tensor/acl/tensor.hpp"

int main(int argc, char **argv)
{
  ////// Parse arguments
  if (argc < 5) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string onnxModelFilePath = argv[1];
  const std::string imagePathPrefix   = argv[2];
  const std::string labelsFilePath    = argv[3];
  uint64_t          imagesToAnalyze   = std::stoull(argv[4]);
  const std::string kernelsPath       = argv[5];

  ////// Initialize acl runtime
  auto err = aclInit(nullptr);
  if (err != ACL_SUCCESS) { HICR_THROW_RUNTIME("Can not init ACL runtime %d", err); }

  err = aclopSetModelDir(kernelsPath.c_str());
  if (err != ACL_SUCCESS) { HICR_THROW_RUNTIME("Can set ACL model directory %d", err); }

  ////// Declare backend-specific HiCR resources
  // Creating HWloc topology object
  hwloc_topology_t hwlocTopology;
  hwloc_topology_init(&hwlocTopology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager  hostTopologyManager(&hwlocTopology);
  HiCR::backend::acl::TopologyManager aclTopologyManager;

  HiCR::backend::acl::MemoryManager        aclMemoryManager;
  HiCR::backend::acl::CommunicationManager aclCommunicationManager;
  HiCR::backend::acl::ComputeManager       aclComputeManager;

  // Asking backend to check the available devices
  auto hostTopology   = hostTopologyManager.queryTopology();
  auto deviceTopology = aclTopologyManager.queryTopology();

  // Getting first device found in the topology
  auto host   = *hostTopology.getDevices().begin();
  auto device = *deviceTopology.getDevices().begin();

  // Getting memory spaces and pick the first one found
  auto hostMemorySpaces       = host->getMemorySpaceList();
  auto deviceMemorySpaces     = device->getMemorySpaceList();
  auto deviceComputeResources = device->getComputeResourceList();
  auto hostMemorySpace        = *hostMemorySpaces.begin();
  auto deviceMemorySpace      = *deviceMemorySpaces.begin();
  auto deviceComputeResource  = *deviceComputeResources.begin();

  auto deviceProcessingUnit = aclComputeManager.createProcessingUnit(deviceComputeResource);

  // Create execution unit factory
  auto executioUnitFactory = factory::acl::ExecutionUnitFactory(aclComputeManager, aclCommunicationManager, aclMemoryManager, deviceMemorySpace, hostMemorySpace);

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

  uint64_t failures = 0;

  for (uint64_t i = 0; i < imagesToAnalyze; i++)
  {
    // Create the neural network
    auto neuralNetwork = NeuralNetwork(aclComputeManager,
                                       std::move(deviceProcessingUnit),
                                       aclCommunicationManager,
                                       aclMemoryManager,
                                       deviceMemorySpace,
                                       executioUnitFactory,
                                       tensor::acl::Tensor::create,
                                       tensor::acl::Tensor::clone);

    // Load data of the pre-trained model
    neuralNetwork.loadPreTrainedData(model, hostMemorySpace);

    // Create the imageTensor
    auto imageFilePath = imagePathPrefix + "/image_" + std::to_string(i) + ".bin";
    auto imageTensor   = loadImage(imageFilePath, aclCommunicationManager, aclMemoryManager, hostMemorySpace, deviceMemorySpace, tensor::acl::Tensor::create);

    // Run the inference on the imageTensor
    const auto output = neuralNetwork.forward(imageTensor);

    deviceProcessingUnit = neuralNetwork.releaseProcessingUnit();

    auto o = dynamic_pointer_cast<tensor::acl::Tensor>(output);
    if (o == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to supported type"); }

    auto hostOutputTensor = o->toHost(aclMemoryManager, aclCommunicationManager, hostMemorySpace);

    auto desiredPrediction = labels[i];
    auto actualPrediction  = neuralNetwork.getPrediction(hostOutputTensor, output->size());

    if (desiredPrediction != actualPrediction) { failures++; }

    if (i == 0) { printf("img-0 score: %.9f\n", ((const float *)hostOutputTensor->getPointer())[actualPrediction]); }

    aclMemoryManager.freeLocalMemorySlot(hostOutputTensor);

    // Free the input image tensor
    aclMemoryManager.freeLocalMemorySlot(imageTensor->getData());

    if (i % 100 == 0 && i > 0) { printf("Analyzed images: %lu/%lu\n", i, labels.size()); }
  }

  printf("Total failures: %lu/%lu\n", failures, imagesToAnalyze);

  err = aclFinalize();
  if (err != ACL_SUCCESS) { HICR_THROW_RUNTIME("Can not finalize ACL runtime %d", err); }

  //Destroy hwloc topology object
  hwloc_topology_destroy(hwlocTopology);

  return 0;
}
