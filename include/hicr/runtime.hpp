#pragma once

#include <hicr/logger.hpp>
#include <hicr/backend.hpp>

#ifdef HICR_ENABLE_BACKEND_PTHREADS
#include <hicr/backends/pthreads.hpp>
#endif

namespace HiCR
{

class Runtime
{
 public:

  Runtime() = default;
  ~Runtime() = default;

  void initialize()
  {
   // Initializing backends
   #ifdef HICR_ENABLE_BACKEND_PTHREADS
	  _backends.push_back(new backends::PThreads());
   #endif

	  _initialized = true;
  }

  inline std::vector<Backend*>& getBackends()
  {
   if (_initialized == false) LOG_ERROR("Attempting to use HiCR without first initializing it.");

   return _backends;
  }

 private:

  bool _initialized = false;

  std::vector<Backend*> _backends;

};

} // namespace HiCR

