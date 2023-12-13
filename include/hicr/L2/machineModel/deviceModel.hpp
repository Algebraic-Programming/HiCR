/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceModel.hpp
 * @brief Defines the lower level Device Model
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

#include <hicr/L0/computeResource.hpp>
#include <hicr/L2/machineModel/computeResource.hpp>
#include <hicr/L2/machineModel/memorySpace.hpp>
#include <nlohmann_json/json.hpp>

namespace HiCR
{

namespace L2
{

namespace machineModel
{

/**
 * Abstract class definition of a Device object.
 *
 * A Device, depending on type, may contain one or more
 * Compute Resources, and one or more addressable Memory Spaces
 */
class DeviceModel
{
  protected:

  /**
   *  Compute Manager to interface with the device backend
   */
  L1::ComputeManager *_computeManager;

  /**
   *  Memory Manager to interface with the device backend
   */
  L1::MemoryManager *_memoryManager;

  /**
   *  List of actual processing elements
   */
  std::map<HiCR::L0::ComputeResource*, L2::machineModel::ComputeResource *> _computeResources;

  /**
   *  List of memories/NUMA nodes
   */
  std::map<L0::MemorySpace*, MemorySpace *> _memorySpaces;

  /**
   *  Friendly device description
   */
  std::string _type;

  /**
   *  Optional; friendly device name to print, if available
   */
  std::string _name;

  public:

  /**
   * Initialize the device; it is expected that each device will explicitly do specific
   * operations, pick and initialize the correct Managers etc.
   */
  virtual void initialize()
  {
  }

  virtual ~DeviceModel() = default;

  /**
   * Obtains a type description of the device
   *
   * \return The device type in string format
   */
  inline std::string getType() const
  {
    return _type;
  }

  /**
   * Obtains the number of available compute resources in the device
   *
   * \return Number of Compute Resources
   */
  inline size_t getComputeCount() const
  {
    return _computeResources.size();
  }

  /**
   * Obtains the number of available memory spaces detected in the device
   *
   * \return Number of Memory Spaces
   */
  inline size_t getNumMemSpaces() const
  {
    return _memorySpaces.size();
  }

  /**
   * Obtains the set of memory spaces on the device
   *
   * \return An std::set of pointers to the MemorySpaces
   */
  inline std::set<MemorySpace *> getMemorySpaces() const
  {
    std::set<MemorySpace *> ret;
    for (auto it : _memorySpaces)
      ret.insert(it.second);

    return ret;
  }

  /**
   * Obtains the set of compute resources on the device
   *
   * \return An std::set of pointers to the ComputeResources
   */
  inline std::set<L2::machineModel::ComputeResource *> getComputeResources() const
  {
    std::set<L2::machineModel::ComputeResource *> ret;
    for (auto it : _computeResources)
      ret.insert(it.second);

    return ret;
  }

  /**
   * Device specific implementation of the JSON serialization
   *
   * @param[in] json The json-formatted data to parse
   */
  virtual void jSerializeImpl(nlohmann::json &json) = 0;

  /**
   * Creates a JSON description of the device resources.
   * To be used for centralized representation of the unified machine model
   *
   * \return A JSON object using the nlohmann library containing all fields of internal objects (compute units, memory spaces etc.)
   */
  nlohmann::json jSerialize()
  {
    nlohmann::json ret;

    ret["Device Type"] = getType();

    jSerializeImpl(ret);

    return ret;
  }

  /**
   * Clean-up resources
   *
   * Currently called through the MachineModel destructor
   */
  virtual void shutdown()
  {
    for (auto it : _memorySpaces)
      delete it.second;

    for (auto it : _computeResources)
      delete it.second;

    delete _memoryManager;
    delete _computeManager;
  }

}; // class DeviceModel

} // namespace machineModel

} // namespace L2

} // namespace HiCR
