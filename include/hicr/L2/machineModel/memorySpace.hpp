/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Defines the MemorySpace object, to
 * be used in the Device Model.
 * @author O. Korakitis
 * @date 27/10/2023
 */
#pragma once

#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

namespace machineModel
{

/**
 * Class definition for an addressable Memory Space.
 *
 * A Device object may comprise one or more such Memory Spaces,
 * on which data can be allocated, copied and communicated among
 * different Memory Spaces, provided there is connectivity
 */
class MemorySpace
{
  protected:

  /**
   *  Backend-provided unique ID of the MemorySpace
   */
  backend::MemoryManager::memorySpaceId_t _id;

  /**
   * Type for the memory space
   */
  std::string _type;

  /**
   *  Size in Bytes
   */
  size_t _size;

  /**
   * (Optional) Page size in bytes
   */
  size_t _pageSize;

  /**
   *  List of associated processing elements
   */
  backend::ComputeManager::computeResourceList_t _computeResources;

  public:

  /**
   * Disabled default constructor
   */
  MemorySpace() = delete;

  /**
   * Parametric constructor for the memory space object
   *
   * @param[in] id Identifier for the memory space
   * @param[in] type Type for the memory space
   * @param[in] size Size of the memory space
   * @param[in] pageSize Detected page size of the memory space
   */
  MemorySpace(
    backend::MemoryManager::memorySpaceId_t id,
    std::string type,
    size_t size,
    size_t pageSize = 4096) : /* Default page size; Consider using a constant but in a not dangerous way (e.g. define DEFAULT_PAGESIZE could mess up things) */
                              _id(id),
                              _type(type),
                              _size(size),
                              _pageSize(pageSize)
  {
  }

  /**
   * Accessor for the id value
   *
   * @return The id value
   */
  inline backend::MemoryManager::memorySpaceId_t getId() const
  {
    return _id;
  }

  /**
   * Accessor for the type value
   *
   * @return The type value
   */
  inline std::string getType() const
  {
    return _type;
  }

  /**
   * Accessor for the size value
   *
   * @return The size value
   */
  inline size_t getSize() const
  {
    return _size;
  }

  /**
   *  If supported, obtain the amount of memory currently in use.
   * In conjunction with the total size above, the user may deduce
   * information like, usage%, if a particular allocation will be
   * possible etc.
   *
   * @return The usage for this memory space
   */
  inline size_t getUsage() const
  {
    return 0; // TODO
  }

  /**
   * Returns the associated compute units to this memory space
   *
   * @return A list of associated compute units
   */
  inline backend::ComputeManager::computeResourceList_t getComputeUnits() const
  {
    return _computeResources;
  }

  /**
   * Adds a compute resource to this memory space
   *
   * @param[in] id The id of the compute resource to add
   */
  inline void addComputeResource(HiCR::L0::computeResourceId_t id)
  {
    _computeResources.insert(id);
  }

}; // class MemorySpace

} // namespace machineModel

} // namespace HiCR
