/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * @brief Provides a definition for the HiCR Instance class.
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <hicr/definitions.hpp>

namespace HiCR
{

namespace L0
{

/**
 * Defines the instance class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Instances may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Instance
{
  public:

  /**
   * Type definition for a unique instance identifier
   */
  typedef uint64_t instanceId_t;

  /**
   * Default destructor
   */
  virtual ~Instance() = default;

  /**
   * This function returns the (hopefully) unique identifier of the current instance
   * @return The instance identifier
   */
  __USED__ inline instanceId_t getId() const { return _id; }

  protected:

  /**
   * Protected constructor for the base instance class.
   *
   * \note This is a purely abstract class
   *
   * \param[in] id Identifier to assign to this instance
   *
   */
  __USED__ Instance(instanceId_t id) : _id(id){};

  private:

  /**
   * Instance Identifier
   */
  const instanceId_t _id;
};

} // namespace L0

} // namespace HiCR
