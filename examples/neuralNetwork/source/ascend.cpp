#include <acl/acl.h>
#include <chrono>
#include <cstdio>

#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include <hicr/backends/ascend/computeResource.hpp>
#include <hicr/backends/ascend/memorySpace.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/topologyManager.hpp>
#include <hicr/backends/ascend/communicationManager.hpp>
#include <hicr/backends/ascend/computeManager.hpp>
#include <hicr/core/exceptions.hpp>

#include "./include/network.hpp"
#include "./include/imageLoader.hpp"
#include "./include/factory/executionUnit/ascend/executionUnitFactory.hpp"
#include "./include/tensor/ascend/tensor.hpp"

int main(int argc, char **argv)
{
  ////// Parse arguments
  if (argc < 5) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string onnxModelFilePath = argv[1];
  const std::string imagePathPrefix   = argv[2];
  const std::string kernelsPath       = argv[3];
  const std::string labelsFilePath    = argv[4];
  uint64_t          imagesToAnalyze   = std::stoull(argv[5]);

  ////// Initialize Ascend Computing Language runtime
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
  HiCR::backend::ascend::TopologyManager ascendTopologyManager;

  HiCR::backend::ascend::MemoryManager        ascendMemoryManager;
  HiCR::backend::ascend::CommunicationManager ascendCommunicationManager;
  HiCR::backend::ascend::ComputeManager       ascendComputeManager;

  // Asking backend to check the available devices
  auto hostTopology   = hostTopologyManager.queryTopology();
  auto deviceTopology = ascendTopologyManager.queryTopology();

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

  auto deviceProcessingUnit = ascendComputeManager.createProcessingUnit(deviceComputeResource);

  // Create execution unit factory
  auto executioUnitFactory = factory::ascend::ExecutionUnitFactory(ascendComputeManager, ascendCommunicationManager, ascendMemoryManager, deviceMemorySpace, hostMemorySpace);

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

  auto totalDuration = std::chrono::duration<double>::zero();

  uint64_t failures = 0;

  for (uint64_t i = 0; i < imagesToAnalyze; i++)
  {
    // Create the neural network
    auto neuralNetwork = NeuralNetwork(ascendComputeManager,
                                       std::move(deviceProcessingUnit),
                                       ascendCommunicationManager,
                                       ascendMemoryManager,
                                       deviceMemorySpace,
                                       executioUnitFactory,
                                       tensor::ascend::Tensor::create,
                                       tensor::ascend::Tensor::clone);

    // Load data of the pre-trained model
    neuralNetwork.loadPreTrainedData(model, hostMemorySpace);

    // Create the imageTensor
    auto imageFilePath = imagePathPrefix + "/image_" + std::to_string(i) + ".bin";
    auto imageTensor   = loadImage(imageFilePath, ascendCommunicationManager, ascendMemoryManager, hostMemorySpace, deviceMemorySpace, tensor::ascend::Tensor::create);

    // Run the inference on the imageTensor
    auto       start  = std::chrono::high_resolution_clock::now();
    const auto output = neuralNetwork.forward(imageTensor);
    auto       end    = std::chrono::high_resolution_clock::now();
    totalDuration += end - start;

    deviceProcessingUnit = neuralNetwork.releaseProcessingUnit();

    auto o = dynamic_pointer_cast<tensor::ascend::Tensor>(output);
    if (o == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to supported type"); }

    auto hostOutputTensor = o->toHost(ascendMemoryManager, ascendCommunicationManager, hostMemorySpace);

    auto desiredPrediction = labels[i];
    auto actualPrediction  = neuralNetwork.getPrediction(hostOutputTensor, output->size());

    if (desiredPrediction != actualPrediction) { failures++; }

    ascendMemoryManager.freeLocalMemorySlot(hostOutputTensor);

    // Free the input image tensor
    ascendMemoryManager.freeLocalMemorySlot(imageTensor->getData());

    if (i % 100 == 0 && i > 0) { printf("Analyzed images: %lu/%lu\n", i, labels.size()); }
  }

  auto totalExecutionTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(totalDuration).count();
  printf("Total execution time: %ld seconds\n", totalExecutionTimeSeconds);
  printf("Total failures: %lu/%lu\n", failures, imagesToAnalyze);

  err = aclFinalize();
  if (err != ACL_SUCCESS) { HICR_THROW_RUNTIME("Can not finalize ACL runtime %d", err); }

  //Destroy hwloc topology object
  hwloc_topology_destroy(hwlocTopology);

  return 0;
}
