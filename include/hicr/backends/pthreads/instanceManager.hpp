#pragma once

#include <pthread.h>

#include <hicr/core/instanceManager.hpp>

#include "instance.hpp"
#include "instancePool.hpp"

namespace HiCR::backend::pthreads
{

class InstanceManager;

/**
   * Type for any entrypoint function
  */
typedef std::function<void(InstanceManager *)> entryPoint_t;

class InstanceManager final : public HiCR::InstanceManager
{
  public:

  InstanceManager(Instance::instanceId_t rootInstanceId, entryPoint_t entrypoint, InstancePool &instancePool)
    : HiCR::InstanceManager(),
      _rootInstanceId(rootInstanceId),
      _entrypoint(entrypoint),
      _instancePool(instancePool)
  {
    auto currentInstance = std::make_shared<Instance>(pthread_self(), _rootInstanceId);
    setCurrentInstance(currentInstance);
  }

  ~InstanceManager() override = default;

  void detectInstances(size_t initialInstanceCount)
  {
    // Get current instance
    auto currentInstance = getCurrentInstance();

    // Lock the pool
    _instancePool.lock();

    // Add the instance to the pool
    _instancePool.insertInstance(currentInstance);

    // Update the barrier
    _instancePool.updateBarrier(initialInstanceCount);

    // Unlock the pool
    _instancePool.unlock();

    // Wait for all the threads to add their own instance
    _instancePool.barrier();

    // Add all the instances
    for (auto &i : _instancePool.getInstances())
    {
      // Skip current instance
      if (i->getId() != currentInstance->getId()) { addInstance(i); }
    }
  }

  std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) override
  {
    // Storage for the new instance id
    pthread_t newInstanceId;

    // Launch a new pthread executing the entrypoint
    auto status = pthread_create(&newInstanceId, nullptr, launchWrapper, this);
    if (status != 0) { HICR_THROW_RUNTIME("Could not create instance thread. Error: %d", status); }

    // Add to the pool of created pthreads
    _createdThreads.insert(newInstanceId);

    // Create a new HiCR instance
    auto instance = std::make_shared<Instance>(newInstanceId, _rootInstanceId);

    // Lock the pool
    _instancePool.lock();

    // Add the new instance
    _instancePool.insertInstance(instance);

    // Update the barrier
    _instancePool.updateBarrier(_instancePool.getInstances().size());

    // Unlock
    _instancePool.unlock();

    return instance;
  }

  std::shared_ptr<HiCR::Instance> addInstanceImpl(Instance::instanceId_t instanceId) override { return std::make_shared<Instance>(instanceId, _rootInstanceId); }

  void terminateInstanceImpl(const std::shared_ptr<HiCR::Instance> instance) override
  {
    // Lock the pool
    _instancePool.lock();

    // Delete instance
    _instancePool.deleteInstance(instance);

    // Unlock
    _instancePool.unlock();
  }

  void finalize() override
  {
    for (auto threadId : _createdThreads) { pthread_join(threadId, nullptr); }
  }

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
  InstancePool                &_instancePool;

  std::unordered_set<pthread_t> _createdThreads;
};
} // namespace HiCR::backend::pthreads