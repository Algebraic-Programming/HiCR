/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the sequential backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L0
{

/** 
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  MemorySpace(const size_t size) : HiCR::L0::MemorySpace(size) {};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "System RAM"; }
};

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
