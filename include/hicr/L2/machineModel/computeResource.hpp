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

#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L1/computeManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <map>

namespace HiCR
{

namespace L2
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
   *  Backend-specific compute resource pointer
   */
  HiCR::L0::ComputeResource* _computeResource;

  /**
   *  Optional; The device number, or CPU logical ID, if the _computeResource differs/doesn't suffice
   */
  size_t _index;

  /**
   *  Denotes the type of the ComputeResource
   */
  std::string _type;

  /**
   *  List of associated Memory Spaces
   */
  L1::MemoryManager::memorySpaceList_t _memorySpaces;

  /**
   *  Associated HiCR::L0::ProcessingUnit executing on the resource
   */
  HiCR::L0::ProcessingUnit *_procUnit;

  /**
   *  Optional; distances from other NUMA nodes in case of multiple NUMA nodes present
   */
  std::map<L0::MemorySpace*, size_t> _numaDistances;

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
    HiCR::L0::ComputeResource* computeResource,
    std::string type) : _computeResource(computeResource),
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
   * Obtain the compute unit of the resource
   *
   * \return A computeResourceId associated with the ComputeResource
   */
  inline HiCR::L0::ComputeResource* getComputeResource() const
  {
    return _computeResource;
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
  inline L1::MemoryManager::memorySpaceList_t getMemorySpaces() const
  {
    return _memorySpaces;
  }

  /**
   * Add a Memory Space in the set of associated Memory Spaces
   * This should be used only during initialization / resource detection.
   *
   * \param[in] id The ID of the Memory Space to be added
   */
  inline void addMemorySpace(L0::MemorySpace* const memorySpace)
  {
    _memorySpaces.insert(memorySpace);
  }

}; // class ComputeResource

} // namespace machineModel

} // namespace L2

} // namespace HiCR
