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

  /**
   * This function reports whether the caller is the root instance
   *
   * The root instance represents a single instance in the entire deployment with the following characteristics:
   *  - It is unique. Only a single instance shall be root in the entire deployment, even if new ones are created.
   *  - It belongs among the first set of instances created during launch time
   *  - It has no parent instance
   *
   * The purpose of the root instance is to provide the minimal tiebreak mechanism that helps in role/task distribution.
   *
   * \note Each backend decides how to designate the root instance. Hence, this is a pure virtual function.
   *
   * @return True, if the current instance is root; false, otherwise.
   */
  virtual bool isRootInstance() const = 0;

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
