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
 bool isPullFunctionDefined = false;

 public:

 Dispatcher() = default;
 ~Dispatcher() = default;

 inline void setPullFunction(const pullFunction_t pullFc)
 {
  _pullFc = pullFc;
  isPullFunctionDefined = true;
 }

 inline void clearPullFunction()
 {
  isPullFunctionDefined = false;
 }


 inline Task* pull()
 {
  if (isPullFunctionDefined == false) LOG_ERROR("Trying to pull on dispatcher but the pull function is not defined\n");
  return _pullFc();
 }
};


} // namespace HiCR
