/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief Defines the ComputeResource object, to
 * be used in the Device Model.
 * @author O. Korakitis
 * @date 27/10/2023
 */
#pragma once

#include <map>

#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

namespace machineModel
{

/**
 * Class definition for a Compute Resource.
 *
 * A Device object may comprise one or more such Compute Resources,
 * on which tasks (as single deployable objects, e.g. function or kernel)
 * can be executed.
 */
class ComputeResource
{
  protected:

  /**
   *  Backend-provided unique ID of the ComputeResource
   */
  HiCR::L0::computeResourceId_t _id;

  /**
   *  Optional; The device number, or CPU logical ID, if the _id differs/doesn't suffice
   */
  size_t _index;

  /**
   *  Denotes the type of the ComputeResource
   */
  std::string _type;

  /**
   *  List of associated Memory Spaces
   */
  backend::MemoryManager::memorySpaceList_t _memorySpaces;

  /**
   *  Associated HiCR::L0::ProcessingUnit executing on the resource
   */
  HiCR::L0::ProcessingUnit *_procUnit;

  /**
   *  Optional; distances from other NUMA nodes in case of multiple NUMA nodes present
   */
  std::map<backend::MemoryManager::memorySpaceId_t, size_t> _numaDistances;

  public:

  /**
   * Disabled default constructor
   */
  ComputeResource() = delete;

  /**
   * Parametric constuctor for the ComputeResource class
   *
   * @param[in] id Id of the compute resource
   * @param[in] type Type of the compute resource
   */
  ComputeResource(
    HiCR::L0::computeResourceId_t id,
    std::string type) : _id(id),
                        _type(type)
  {
  }

  /**
   * Default destructor
   */
  virtual ~ComputeResource() /* consider doing this in Device shutdown() */
  {
    delete _procUnit;
  }

  /**
   * Obtain the ID of the resource
   *
   * \return A computeResourceId associated with the ComputeResource
   */
  inline HiCR::L0::computeResourceId_t getId() const
  {
    return _id;
  }

  /**
   * Obtain the device index of the resource (possibly redundant, TBD)
   *
   * \return A device index associated with the ComputeResource
   */
  inline size_t getIndex() const
  {
    return _index;
  }

  /**
   * Obtain the type of the resource
   *
   * \return A string describing the device type.
   */
  inline std::string getType() const
  {
    return _type;
  }

  /**
   * Function to get the processing unit out this compute resource
   *
   * @return A pointer to the processing unit
   *
   * TODO: change this to meaningful functionality; currently placeholder
   */
  inline HiCR::L0::ProcessingUnit *getProcessingUnit() const
  {
    return _procUnit;
  }

  /**
   * Obtain a list of the Memory Spaces associated with the device
   *
   * \return A hash set of associated MemorySpaces
   */
  inline backend::MemoryManager::memorySpaceList_t getMemorySpaces() const
  {
    return _memorySpaces;
  }

  /**
   * Add a Memory Space in the set of associated Memory Spaces
   * This should be used only during initialization / resource detection.
   *
   * \param[in] id The ID of the Memory Space to be added
   */
  inline void addMemorySpace(backend::MemoryManager::memorySpaceId_t id)
  {
    _memorySpaces.insert(id);
  }

}; // class ComputeResource

} // namespace machineModel

} // namespace HiCR
