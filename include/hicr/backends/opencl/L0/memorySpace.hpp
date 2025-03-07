/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/backends/opencl/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L0
{

/**
 * Forward declaration of the opencl device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a memory space, as visible by the OpenCL backend. That is, the entire accessible RAM
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the OpenCL backend
   *
   * \param device The opencl device in which this memory space was detected
   * \param type The memory space type
   * \param size The size of this memory space
   */
  MemorySpace(const std::weak_ptr<opencl::L0::Device> device, const std::string &type, const size_t size)
    : HiCR::L0::MemorySpace(size),
      _device(device),
      _type(type) {};

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
  MemorySpace(const nlohmann::json &input)
    : HiCR::L0::MemorySpace()
  {
    deserialize(input);
  }

  __INLINE__ std::string getType() const override { return _type; }

  /**
   * Function to get the opencl device associated to this memory space
   *
   * @return The opencl device corresponding to this memory space
   */
  __INLINE__ const std::weak_ptr<const opencl::L0::Device> getDevice() const { return _device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override { output["Memory Space Type"] = _type; }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string typeKey = "Memory Space Type";
    if (input.contains(typeKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", typeKey.c_str());
    if (input[typeKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", typeKey.c_str());
    _type = input[typeKey].get<std::string>();
  }

  /**
   * Stores the device that owns this memory space
   *
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<opencl::L0::Device> _device;

  /**
   * Memory space device type
  */
  std::string _type;
};

} // namespace L0

} // namespace opencl

} // namespace backend

} // namespace HiCR
