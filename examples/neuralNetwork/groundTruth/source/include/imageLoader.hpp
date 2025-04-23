#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <hicr/core/exceptions.hpp>

#include "./tensor.hpp"


/**
 * Load MNIST labels into a vector
 */
std::vector<uint32_t> loadLabels(const std::string &labelFilePath)
{
  std::ifstream in(labelFilePath, std::ios::binary);
  if (!in) { HICR_THROW_RUNTIME("Cannot open label file: %s", labelFilePath); }

  // Determine the file size
  in.seekg(0, std::ios::end);
  std::streamsize fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  // Determine how many labels should be loaded
  size_t                numLabels = fileSize / sizeof(uint32_t);
  std::vector<uint32_t> labels(numLabels);

  // Read the entire file into the vector
  if (!in.read(reinterpret_cast<char *>(labels.data()), fileSize)) { HICR_THROW_RUNTIME("Error reading label file: %s", labelFilePath); }

  // Close file
  in.close();

  return labels;
}

/**
 * Load an image inside a tensor
 * 
 * @param[in] inputFilePath path to the image file
 * 
 * @return a tensor containig the image
*/
tensor_t loadImage(const std::string &inputFilePath)
{
  // Create a stream of the file
  std::ifstream file(inputFilePath, std::ios::binary);
  if (!file) { HICR_THROW_RUNTIME("Can not open image file"); }

  // Find out the size of the file
  file.seekg(0, std::ios::end);
  size_t fileSize = file.tellg();

  // Go back to the beginning of the file
  file.seekg(0, std::ios::beg);

  // Read the file into a vector of chars
  auto fileData = std::vector<char>(fileSize, '\0');
  file.read(fileData.data(), fileSize);

  // Compute how many pixels are contained in the image and store them in a vector of float
  size_t pixelCount     = fileSize / sizeof(float);
  auto   floatImageData = reinterpret_cast<float *>(fileData.data());
  auto   pixelValues    = std::vector<float>(floatImageData, floatImageData + pixelCount);

  // Close the file
  file.close();

  // Create the shape vector and then the tensor
  // Known a priori
  auto shape = std::vector<uint64_t>({28, 28});

  return tensor_t(shape, pixelValues);
}
