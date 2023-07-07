#pragma once

#include <hicr/logger.hpp>
#include <hicr/common.hpp>

namespace HiCR
{

class Event
{
 public:

  Event(eventCallback_t& fc) : _fc(fc) {}
  ~Event() = default;

  inline void trigger(taskId_t taskId)
  {
    _fc(taskId);
  }

 private:

  eventCallback_t _fc;
};

} // namespace HiCR

