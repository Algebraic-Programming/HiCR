#include <chrono>
#include <cstdio>

#include <hicr/backends/hwloc/computeResource.hpp>
#include <hicr/backends/hwloc/memorySpace.hpp>
#include <hicr/backends/hwloc/memoryManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/pthreads/communicationManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/core/exceptions.hpp>

#include "./include/network.hpp"
#include "./include/imageLoader.hpp"
#include "./include/factory/executionUnit/pthreads/executionUnitFactory.hpp"
#include "./include/tensor/pthreads/tensor.hpp"

int main(int argc, char **argv)
{
  ////// Parse arguments
  if (argc < 4) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string onnxModelFilePath = argv[1];
  const std::string imagePathPrefix   = argv[2];
  const std::string labelsFilePath    = argv[3];
  uint64_t          imagesToAnalyze   = std::stoull(argv[4]);

  ////// Declare backend-specific HiCR resources
  // Creating HWloc topology object
  hwloc_topology_t hwlocTopology;
  hwloc_topology_init(&hwlocTopology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager         topologyManager(&hwlocTopology);
  HiCR::backend::hwloc::MemoryManager           memoryManager(&hwlocTopology);
  HiCR::backend::pthreads::CommunicationManager communicationManager;
  HiCR::backend::pthreads::ComputeManager       computeManager;

  // Asking backend to check the available devices
  auto topology = topologyManager.queryTopology();

  // Getting first device found in the topology
  auto device = *topology.getDevices().begin();

  // Getting compute resources and pick the first one found
  auto computeResources    = device->getComputeResourceList();
  auto hostComputeResource = *computeResources.begin();

  auto hostProcessingUnit = computeManager.createProcessingUnit(hostComputeResource);

  // Getting memory spaces and pick the first one found
  auto memorySpaces    = device->getMemorySpaceList();
  auto hostMemorySpace = *memorySpaces.begin();

  // Create execution unit factory
  auto executioUnitFactory = factory::pthreads::ExecutionUnitFactory(computeManager);

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

  auto     totalDuration = std::chrono::duration<double>::zero();
  uint64_t failures      = 0;

  for (uint64_t i = 0; i < imagesToAnalyze; i++)
  {
    // Create the neural network
    auto neuralNetwork = NeuralNetwork(computeManager,
                                       std::move(hostProcessingUnit),
                                       communicationManager,
                                       memoryManager,
                                       hostMemorySpace,
                                       executioUnitFactory,
                                       tensor::pthreads::Tensor::create,
                                       tensor::pthreads::Tensor::clone);

    // Load data of the pre-trained model
    neuralNetwork.loadPreTrainedData(model, hostMemorySpace);

    // Create the imageTensor
    auto imageFilePath = imagePathPrefix + "/image_" + std::to_string(i) + ".bin";
    auto imageTensor   = loadImage(imageFilePath, communicationManager, memoryManager, hostMemorySpace, hostMemorySpace, tensor::pthreads::Tensor::create);

    // Run the inference on the imageTensor
    auto       start  = std::chrono::high_resolution_clock::now();
    const auto output = neuralNetwork.forward(imageTensor);
    auto       end    = std::chrono::high_resolution_clock::now();
    totalDuration += end - start;

    hostProcessingUnit = neuralNetwork.releaseProcessingUnit();

    auto desiredPrediction = labels[i];
    auto actualPrediction  = neuralNetwork.getPrediction(output->getData(), output->size());

    if (desiredPrediction != actualPrediction) { failures++; }

    // Free the input image tensor
    memoryManager.freeLocalMemorySlot(imageTensor->getData());

    if (i % 100 == 0 && i > 0) { printf("Analyzed images: %lu/%lu\n", i, labels.size()); }
  }

  auto totalExecutionTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(totalDuration).count();
  printf("Total execution time: %ld seconds\n", totalExecutionTimeSeconds);
  printf("Total failures: %lu/%lu\n", failures, imagesToAnalyze);

  //Destroy hwloc topology object
  hwloc_topology_destroy(hwlocTopology);

  return 0;
}
