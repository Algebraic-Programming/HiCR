/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the host (CPU) backends
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/memorySpace.hpp>

namespace HiCR::backend::host::L0
{

/**
 * This class represents a segment of addressable memory space within a NUMA domain
 */
class MemorySpace : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the memory space class of the shared memory backend
   *
   * \param size The maximum allocatable size detected for this memory space
   */
  MemorySpace(const size_t size)
    : HiCR::L0::MemorySpace(size)
  {}

  /**
   * Default destructor
   */
  ~MemorySpace() override = default;

  [[nodiscard]] __INLINE__ std::string getType() const override { return "RAM"; }

  protected:

  /**
   * Default constructor for deserialization purposes
   */
  MemorySpace() = default;

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // No additional information to serialize for now
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // No additional information to deserialize for now
  }
};

} // namespace HiCR::backend::host::L0
