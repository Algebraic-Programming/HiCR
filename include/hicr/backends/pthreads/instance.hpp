#pragma once

#include <pthread.h>
#include <memory>
#include <hicr/core/instance.hpp>

namespace HiCR::backend::pthreads
{

class Instance : public HiCR::Instance
{
  public:

  Instance(instanceId_t currentInstanceId, instanceId_t rootInstanceId)
    : HiCR::Instance(currentInstanceId),
      _rootInstanceId(rootInstanceId){};

  ~Instance() = default;

  bool isRootInstance() const override { return getId() != _rootInstanceId; };

  private:

  instanceId_t _rootInstanceId;
};
} // namespace HiCR::backend::pthreads