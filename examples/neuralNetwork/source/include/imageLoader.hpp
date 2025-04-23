#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <hicr/core/exceptions.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/memoryManager.hpp>

#include "./tensor/tensor.hpp"

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
 * @param[in] communicationManage HiCR communicaton manager to copy data
 * @param[in] memoryManager HiCR memory manager to register and allocate new local memory slots
 * @param[in] hostMemorySpace The host memory space in which data should be temporarily copied from the file 
 * @param[in] dstMemorySpace The memory space in which data should be copied and used by the application. Can be host or device
 * 
 * @return a tensor containig the image
*/
std::shared_ptr<tensor_t> loadImage(const std::string                            &inputFilePath,
                                    HiCR::CommunicationManager               &communicationManager,
                                    HiCR::MemoryManager                      &memoryManager,
                                    const std::shared_ptr<HiCR::MemorySpace> &hostMemorySpace,
                                    const std::shared_ptr<HiCR::MemorySpace> &dstMemorySpace,
                                    tensorFactoryFc_t                             tensorFactoryFunction)
{
  // Create a stream of the file
  std::ifstream file(inputFilePath, std::ios::binary);
  if (!file) { HICR_THROW_RUNTIME("Can not open image file %s", inputFilePath.c_str()); }

  // Find out the size of the file
  file.seekg(0, std::ios::end);
  size_t fileSize = file.tellg();

  // Go back to the beginning of the file
  file.seekg(0, std::ios::beg);

  // Read the file into the host memory space
  auto hostMemSlot = memoryManager.allocateLocalMemorySlot(hostMemorySpace, fileSize);
  file.read((char *)hostMemSlot->getPointer(), fileSize);

  // Copy data into the destination memory space
  auto dstMemSlot = memoryManager.allocateLocalMemorySlot(dstMemorySpace, fileSize);
  communicationManager.memcpy(dstMemSlot, 0, hostMemSlot, 0, hostMemSlot->getSize());

  // Close the file
  file.close();

  // Free temporary memory slot
  memoryManager.freeLocalMemorySlot(hostMemSlot);

  // Create the shape vector and then the tensor
  // Known a priori
  auto shape = std::vector<uint64_t>({1, 28 * 28});

  return tensorFactoryFunction(shape, dstMemSlot);
}
