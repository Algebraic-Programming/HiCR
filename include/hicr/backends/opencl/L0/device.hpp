/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/device.hpp>
#include <hicr/backends/opencl/L0/computeResource.hpp>
#include <hicr/backends/opencl/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L0
{

/**
 * This class represents a device, as visible by the OpenCL backend.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Type definition for the OpenCL Device identifier
   */
  typedef uint64_t deviceIdentifier_t;

  /**
   * Constructor for an OpenCL device
   *
   * \param id Internal unique identifier for the device
   * \param type Device type
   * \param device OpenCL device class
   * \param computeResources The compute resources associated to this device (typically just one, the main OpenCL processor)
   * \param memorySpaces The memory spaces associated to this device (DRAM + other use-specific or high-bandwidth memories)
   */
  Device(const deviceIdentifier_t           id,
         const std::string                 &type,
         const std::shared_ptr<cl::Device> &device,
         const computeResourceList_t       &computeResources,
         const memorySpaceList_t           &memorySpaces)
    : HiCR::L0::Device(computeResources, memorySpaces),
      _id(id),
      _type(type),
      _device(device) {};

  /**
   * Default constructor for resource requesting
   */
  Device()
    : HiCR::L0::Device()
  {}

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed OpenCL device
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized OpenCL device information
   */
  Device(const nlohmann::json &input)
    : HiCR::L0::Device()
  {
    deserialize(input);
  }

  /**
   * Device destructor
   */
  ~Device() {

  };

  /**
   * Get device id
   * 
   * \return device id
  */
  __INLINE__ deviceIdentifier_t getId() const { return _id; }

  /**
   * Get device type 
   * 
   * \return device type 
  */
  __INLINE__ std::string getType() const override { return _type; }

  /**
   * Get OpenCL Device 
   * 
   * \return OpenCL Device
  */
  __INLINE__ const cl::Device &getOpenCLDevice() const { return *_device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    output["Device Identifier"] = _id;
    output["Device Type"]       = _type;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string idKey   = "Device Identifier";
    std::string typeKey = "Device Type";

    if (input.contains(idKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", idKey.c_str());
    if (input[idKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", idKey.c_str());

    if (input.contains(typeKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", typeKey.c_str());
    if (input[typeKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", typeKey.c_str());

    _id   = input[idKey].get<deviceIdentifier_t>();
    _type = input[typeKey].get<std::string>();

    for (const auto &computeResource : input["Compute Resources"])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != getType() + " Processing Unit") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<opencl::L0::ComputeResource>(computeResource);

      // Inserting device into the list
      this->addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input["Memory Spaces"])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != getType() + " RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<opencl::L0::MemorySpace>(memorySpace);

      // Inserting device into the list
      this->addMemorySpace(memorySpaceObj);
    }
  }

  /**
   * Individual identifier for the opencl device
   */
  deviceIdentifier_t _id;

  /**
   * String reprensenting the device type
  */
  std::string _type;

  /**
   * OpenCL device
  */
  const std::shared_ptr<cl::Device> _device;
};

} // namespace L0

} // namespace opencl

} // namespace backend

} // namespace HiCR
