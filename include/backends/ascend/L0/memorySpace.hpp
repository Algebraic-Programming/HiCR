/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 15/12/2023
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/definitions.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <backends/ascend/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * Forward declaration of the ascend device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   *
   * \param device The ascend device in which this memory space was detected
   * \param size The size of this memory space
   */
  MemorySpace(const std::weak_ptr<ascend::L0::Device> device, const size_t size) : HiCR::L0::MemorySpace(size), _device(device){};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  /**
  * Deserializing constructor
  * 
  * @param[in] input Serialized resource information
  * 
  * \note Backwards reference to device is null when deserializing. Do not try to use this class for any operations.
  */
  MemorySpace(const nlohmann::json& input) : HiCR::L0::MemorySpace()
  {
    deserialize(input);
  }

  __USED__ inline std::string getType() const override { return "Ascend Device RAM"; }

  /**
   * Function to get the ascend device associated to this memory space
   *
   * @return The ascend device corresponding to this memory space
   */
  __USED__ inline const std::weak_ptr<const ascend::L0::Device> getDevice() const { return _device; }

  private:

  __USED__ inline void serializeImpl(nlohmann::json& output) const override
  {
     // No additional information to serialize for now
  }

  __USED__ inline void deserializeImpl(const nlohmann::json& input) override
  {
    // No additional information to deserialize for now
  }


  /**
   * Stores the device that owns this memory space
   * 
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<ascend::L0::Device> _device;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
