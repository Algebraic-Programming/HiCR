#pragma once

#include <vector>
#include <hicr/task.hpp>

namespace HiCR
{

typedef std::function<Task*(void)> pullFunction_t;

class Dispatcher
{
 private:

 // Pull function, if defined by the user
 pullFunction_t _pullFc;

 public:

 Dispatcher(const pullFunction_t pullFc) : _pullFc {pullFc} { };
 ~Dispatcher() = default;

 inline void setPullFunction(const pullFunction_t pullFc)
 {
  _pullFc = pullFc;
 }

 inline Task* pull()
 {
  return _pullFc();
 }
};


} // namespace HiCR
