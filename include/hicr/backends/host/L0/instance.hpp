/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the Host backend
 * @author S. M. Martin
 * @date 4/4/2024
 */
#pragma once

#include <hicr/core/L0/instance.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace L0
{

/**
 * This class represents an abstract definition for a HICR instance as represented by the Host backend
 */
class Instance final : public HiCR::L0::Instance
{
  public:

  /**
   * Constructor for a Instance class for the Host backend
   */
  Instance() : HiCR::L0::Instance(0)
  {}

  /**
   * Default destructor
   */
  ~Instance() = default;

  virtual bool isRootInstance() const
  {
    // Only a single instance exists (the currently running one), hence always true
    return true;
  };

};

} // namespace L0

} // namespace host

} // namespace backend

} // namespace HiCR
