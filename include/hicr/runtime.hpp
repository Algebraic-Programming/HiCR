#pragma once

namespace HiCR
{

class Runtime
{
 public:

  Runtime() = default;
  ~Runtime() = default;

  void initialize()
  {
   LOG_DEBUG("HiCR Initialized\n");
  }

};
} // namespace HiCR

