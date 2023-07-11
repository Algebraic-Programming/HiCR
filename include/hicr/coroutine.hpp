#pragma once

#include <boost/context/continuation.hpp>
#include <hicr/common.hpp>

namespace HiCR
{

class Coroutine
{
 public:

  Coroutine() = default;
  ~Coroutine() = default;

  inline void resume()
  {
   _contextSource = _contextSource.resume();
  }

  inline void yield()
  {
   _contextSink = _contextSink.resume();
  }

  inline void start (taskFunction_t& fc, void* arg)
  {
    auto coroutineFc = [this, fc, arg](boost::context::continuation &&sink)
    {
     _contextSink = std::move(sink);
     fc(arg);
     yield();
     return std::move(sink);
    };

    // Creating new context
    _contextSource = boost::context::callcc(coroutineFc);
  }

 private:

  // CPU execution context of the current rank and worker thread.
  boost::context::continuation _contextSource;
  boost::context::continuation _contextSink;
};

} // namespace HiCR

