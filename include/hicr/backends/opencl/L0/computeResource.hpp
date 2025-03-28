/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for the OpenCL backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/computeResource.hpp>

namespace HiCR::backend::opencl::L0
{

/**
 * Forward declaration of the OpenCL device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a compute resource, visible by the OpenCL backend.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the OpenCL backend
   *
   * \param device The OpenCL device that contains this compute resource
   * \param type device resource type
   */
  ComputeResource(const std::shared_ptr<opencl::L0::Device> &device, const std::string &type)
    : HiCR::L0::ComputeResource(),
      _device(device),
      _type(type){};

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  /**
   * Deserializing constructor
   *
   * @param[in] input Serialized resource information
   *
   * \note Backwards reference to device is null when deserializing. Do not try to use this class for any operations.
   */
  ComputeResource(const nlohmann::json &input)
    : HiCR::L0::ComputeResource()
  {
    deserialize(input);
  }

  __INLINE__ std::string getType() const override { return _type; }

  /**
   * Function to get the device id associated to this memory space
   *
   * @return The device id corresponding to this memory space
   */
  __INLINE__ const std::weak_ptr<const opencl::L0::Device> getDevice() const { return _device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override { output["Compute Resource Type"] = _type; }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string typeKey = "Compute Resource Type";
    if (input.contains(typeKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", typeKey.c_str());
    if (input[typeKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", typeKey.c_str());
    _type = input[typeKey].get<std::string>();
  }

  /**
   * Stores the device that owns this compute resource
   *
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<opencl::L0::Device> _device;

  std::string _type;
};

} // namespace HiCR::backend::opencl::L0
