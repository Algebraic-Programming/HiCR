/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.hpp
 * @brief This file implements the instance manager class for the  HWLoc-based backend (single instance)
 * @author S. M. Martin
 * @date 4/4/2024
 */

#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/backends/hwloc/L0/instance.hpp>
#include <hicr/core/L1/instanceManager.hpp>

namespace HiCR::backend::hwloc::L1
{

/**
 * Implementation of the HiCR hwloc Instance Manager
 */
class InstanceManager final : public HiCR::L1::InstanceManager
{
  public:

  /**
   * Constructor for the hwloc instance manager
   */
  InstanceManager()
    : HiCR::L1::InstanceManager()
  {
    // A single instance (the currently running) is created and is deemed as the root
    auto currentInstance = std::make_shared<HiCR::backend::hwloc::L0::Instance>();
    setCurrentInstance(currentInstance);
    addInstance(currentInstance);
  }

  ~InstanceManager() override = default;

  /**
   * Triggers the execution of the specified RPC (by name) in the specified instance
   *
   * @param[in] instance The instance in which to execute the RPC
   * @param[in] RPCTargetName The name of the target RPC to execute
   *
   */
  __INLINE__ void launchRPC(HiCR::L0::Instance &instance, const std::string &RPCTargetName) const override
  {
    // Calculating index for the RPC target's name
    auto idx = getRPCTargetIndexFromString(RPCTargetName);

    // Executing the RPC directly
    executeRPC(idx);
  }

  __INLINE__ void *getReturnValueImpl(HiCR::L0::Instance &instance) const override
  {
    // Returning buffer containing the return value
    return _returnValueBuffer;
  }

  __INLINE__ void submitReturnValueImpl(const void *pointer, const size_t size) override
  {
    // Allocating buffer locally
    _returnValueBuffer = malloc(size);

    // Copying values to buffer
    memcpy(_returnValueBuffer, pointer, size);
  }

  __INLINE__ void listenImpl() override { HICR_THROW_LOGIC("Calling listen using the Host instance manager results in a deadlock (nobody else to notify us). Aborting."); }

  __INLINE__ std::shared_ptr<HiCR::L0::Instance> createInstanceImpl(const std::shared_ptr<HiCR::L0::InstanceTemplate> &instanceTemplate) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the launching of new instances during runtime");
  }

  __INLINE__ std::shared_ptr<HiCR::L0::Instance> addInstanceImpl(HiCR::L0::Instance::instanceId_t instanceId) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the detection of new instances during runtime");
  }

  __INLINE__ void finalize() override {}

  __INLINE__ void abort(int errorCode) override { std::abort(); }

  /**
   * This function represents the default intializer for this backend
   *
   * @param[in] argc A pointer to the argc value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @param[in] argv A pointer to the argv value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::L1::InstanceManager> createDefault(int *argc, char ***argv)
  {
    // Creating instance manager
    return std::make_unique<HiCR::backend::hwloc::L1::InstanceManager>();
  }

  [[nodiscard]] __INLINE__ HiCR::L0::Instance::instanceId_t getRootInstanceId() const override { return 0; }

  private:

  /**
   * The return value buffer is stored locally 
  */
  void *_returnValueBuffer{};
};

} // namespace HiCR::backend::hwloc::L1
