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

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

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
   * Complete state set that a worker can be in
   */
  enum state_t
  {
    /**
     * The instance is online but not listening (detached mode)
     */
    detached,

    /**
     * The instance is currently running
     */
    running,

    /**
     * The instance is listening for incoming RPCs (attached)
     */
    listening,

    /**
     * The instance has reached its end
     */
    finished
  };

  /**
   * State getter
   * \return The internal state of the instance
   */
  state_t getState() const { return _state; }

  /**
   * Convenience function to print the state as a string
   * \param[in] state The value of the state (enumeration)
   * \return The string corresponding to the passed state
   */
  static std::string getStateString(const state_t state)
  {
    if (state == state_t::detached) return std::string("Detached");
    if (state == state_t::listening) return std::string("Listening");
    if (state == state_t::running) return std::string("Running");
    if (state == state_t::finished) return std::string("Finished");

    HICR_THROW_LOGIC("Unrecognized instance state (0x%lX).\n", state);
  }

  /**
   * This function returns the (hopefully) unique identifier of the current instance
   * @return The instance identifier
   */
  __USED__ inline instanceId_t getId() const { return _id; }

  /**
   * State setter. Only used internally to update the state
   * \param[in] state The new value of the state to be set
   */
  __USED__ inline void setState(const state_t state) { _state = state; }

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

  /**
   * Represents the internal state of the instance. Inactive upon creation.
   */
  state_t _state = state_t::detached;

  private:

  /**
   * Instance Identifier
   */
  const instanceId_t _id;
};

} // namespace L0

} // namespace HiCR
