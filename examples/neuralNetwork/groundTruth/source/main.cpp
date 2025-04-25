#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <hicr/core/exceptions.hpp>

#include "./include/layers.hpp"
#include "./include/network.hpp"
#include "./include/imageLoader.hpp"

int main(int argc, char **argv)
{
  // Parse arguments
  if (argc < 3) { HICR_THROW_RUNTIME("Not enough arguments"); }
  const std::string onnxModelFilePath = argv[1];
  const std::string imagePathPrefix   = argv[2];
  const std::string labelsFilePath    = argv[3];
  uint64_t          imagesToAnalyze   = std::stoull(argv[4]);

  // Load ONNX model
  onnx::ModelProto model;
  std::ifstream    modelFile(onnxModelFilePath, std::ios::binary);
  if (!model.ParseFromIstream(&modelFile)) { HICR_THROW_RUNTIME("Failed to parse the model."); }

  // Load MNIST labels
  auto labels     = loadLabels(labelsFilePath);
  imagesToAnalyze = std::min(imagesToAnalyze, labels.size());

  uint64_t failures = 0;

  for (uint64_t i = 0; i < imagesToAnalyze; i++)
  {
    // Extract the layer information
    auto [tensors, operations] = extractNetworkInformations(model);

    // Create the neural network
    auto neuralNetwork = NeuralNetwork(tensors, operations);

    // Create the imageTensor  // Create the imageTensor
    auto imageFilePath = imagePathPrefix + "/image_" + std::to_string(i) + ".bin";
    auto imageTensor   = loadImage(imageFilePath);

    // Run the inference on the imageTensor
    const auto output = neuralNetwork.forward(imageTensor);

    auto desiredPrediction = labels[i];
    auto actualPrediction  = output.indexOfMax();

    if (desiredPrediction != actualPrediction) { failures++; }

    if (i == 0) { printf("img-0 score: %.9f\n", output.toCFloat()[actualPrediction]); }

    if (i % 100 == 0 && i > 0) { printf("Analyzed images: %lu/%lu\n", i, labels.size()); }
  }

  printf("Total failures: %lu/%lu\n", failures, imagesToAnalyze);

  return 0;
}
