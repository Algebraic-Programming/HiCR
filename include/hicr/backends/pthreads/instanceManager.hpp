#pragma once

#include <pthread.h>

#include <hicr/core/instanceManager.hpp>

#include "instance.hpp"

namespace HiCR::backend::pthreads
{

class InstanceManager;

/**
   * Type for any entrypoint function
  */
typedef std::function<void(InstanceManager *)> entryPoint_t;

class InstanceManager : public HiCR::InstanceManager
{
  public:

  InstanceManager(Instance::instanceId_t rootInstanceId, entryPoint_t entrypoint)
    : HiCR::InstanceManager(),
      _rootInstanceId(rootInstanceId),
      _entrypoint(entrypoint)
  {
    setCurrentInstance(std::make_shared<Instance>(pthread_self(), rootInstanceId));
  }

  ~InstanceManager() = default;

  std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) override
  {
    Instance::instanceId_t newInstanceId;
    auto                   status = pthread_create(&newInstanceId, nullptr, launchWrapper, this);
    if (status != 0) { HICR_THROW_RUNTIME("Could not create instance thread. Error: %d", status); }
    return std::make_shared<Instance>(newInstanceId, _rootInstanceId);
  }

  std::shared_ptr<HiCR::Instance> addInstanceImpl(Instance::instanceId_t instanceId) override { return std::make_shared<Instance>(instanceId, _rootInstanceId); }

  void terminateInstanceImpl(const std::shared_ptr<HiCR::Instance> instance) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the termination of instances during runtime");
  }

  void finalize() override {}
  void abort(int errorCode) override { exit(errorCode); }

  HiCR::Instance::instanceId_t getRootInstanceId() const override { return _rootInstanceId; }

  entryPoint_t getEntrypoint() const { return _entrypoint; }

  private:

  __INLINE__ static void *launchWrapper(void *im)
  {
    auto instanceManager = static_cast<InstanceManager *>(im);
    instanceManager->_entrypoint(instanceManager);
    return 0;
  }

  HiCR::Instance::instanceId_t _rootInstanceId;
  entryPoint_t                 _entrypoint;
};
} // namespace HiCR::backend::pthreads