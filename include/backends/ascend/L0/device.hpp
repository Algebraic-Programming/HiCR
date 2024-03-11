/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 20/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/definitions.hpp>
#include <hicr/L0/device.hpp>
#include <backends/ascend/L0/computeResource.hpp>
#include <backends/ascend/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * This class represents a device, as visible by the shared memory backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Type definition for the Ascend Device identifier
   */
  typedef uint64_t deviceIdentifier_t;

  /**
   * Constructor for an Ascend device
   *
   * \param id Internal unique identifier for the device
   * \param computeResources The compute resources associated to this device (typically just one, the main Ascend processor)
   * \param memorySpaces The memory spaces associated to this device (DRAM + other use-specific or high-bandwidth memories)
   */
  Device(const deviceIdentifier_t id, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : HiCR::L0::Device(computeResources, memorySpaces),
      _id(id),
      _context(std::make_unique<aclrtContext>())
  {
    // create ACL context for executing operations on the given device
    aclError err = aclrtCreateContext(_context.get(), id);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context for device %ld. Error %d", _id, err);
  };

  /**
   * Default constructor for resource requesting
   */
  Device()
    : HiCR::L0::Device()
  {}

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed Ascend device
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized Ascend device information
   */
  Device(const nlohmann::json &input)
    : HiCR::L0::Device()
  {
    deserialize(input);
  }

  /**
   * Set the device on which the operations needs to be executed
   *
   * \param deviceContext the device ACL context
   * \param deviceId the device identifier
   */
  __USED__ static inline void selectDevice(const aclrtContext *deviceContext, const deviceIdentifier_t deviceId)
  {
    // select the device context on which operations shoud be executed
    aclError err = aclrtSetCurrentContext(*deviceContext);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set the device %ld context. Error %d", deviceId, err);
  }

  /**
   * Set this device as the one on which the operations needs to be executed
   */
  __USED__ inline void select() const { selectDevice(_context.get(), _id); }

  /**
   * Device destructor
   */
  ~Device()
  {
    // destroy the ACL context
    aclError err = aclrtDestroyContext(*_context.get());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy context for device %ld. Error %d", _id, err);
  };

  __USED__ inline std::string getType() const override { return "Ascend Device"; }

  /**
   * Returns the internal id of the current Ascend device
   *
   * \return The id of the ascend device
   */
  __USED__ inline deviceIdentifier_t getId() const { return _id; }

  /**
   * Returns the ACL context corresponding to this compute resource
   *
   * \return The ACL context
   */
  __USED__ inline aclrtContext *getContext() const { return _context.get(); }

  private:

  __USED__ inline void serializeImpl(nlohmann::json &output) const override
  {
    // Storing device identifier
    output["Device Identifier"] = _id;
  }

  __USED__ inline void deserializeImpl(const nlohmann::json &input) override
  {
    // Getting device id
    std::string key = "Device Identifier";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _id = input[key].get<deviceIdentifier_t>();

    // Iterating over the compute resource list
    for (const auto &computeResource : input["Compute Resources"])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Ascend Processor") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<ascend::L0::ComputeResource>(computeResource);

      // Inserting device into the list
      _computeResources.insert(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input["Memory Spaces"])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Ascend Device RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<ascend::L0::MemorySpace>(memorySpace);

      // Inserting device into the list
      _memorySpaces.insert(memorySpaceObj);
    }
  }

  /**
   * Individual identifier for the ascend device
   */
  deviceIdentifier_t _id;

  /**
   * The internal Ascend context associated to the device
   */
  const std::unique_ptr<aclrtContext> _context;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
