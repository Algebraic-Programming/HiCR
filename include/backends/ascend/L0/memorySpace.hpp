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

#include <backends/ascend/common.hpp>
#include <backends/ascend/L0/localMemorySlot.hpp>
#include <hicr/L0/memorySpace.hpp> 
#include <hicr/common/definitions.hpp>

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
   */
  MemorySpace(const size_t size) :
   HiCR::L0::MemorySpace(size)
     {};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "Ascend Device RAM"; }

  /**
  * Function to get the ascend device associated to this memory space
  *
  * @return The ascend device corresponding to this memory space
  */
  __USED__ inline const ascend::L0::Device* getDevice() const  { return _device; }

  /**
  * Function to set the ascend device associated to this memory space
  */
  __USED__ inline void setDevice(const ascend::L0::Device* device)  { _device = device; }

  private:

  /**
   * Stores the device that owns this memory space
  */
 const ascend::L0::Device* _device;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
