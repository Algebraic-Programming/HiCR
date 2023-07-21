#pragma once

#include <hicr/logger.hpp>
#include <hicr/backend.hpp>

#ifdef HICR_ENABLE_BACKEND_PTHREADS
#include <hicr/backends/pthreads.hpp>
#endif

namespace HiCR
{


/**
 * @brief Main HiCR runtime system class
 *
 * It can be used to query the system's available resources and their connectivity.
 */
class Runtime
{
 public:

  Runtime() = default;
  ~Runtime() = default;

  /**
   * Detects the backends selected at compilation time.
   */
  void initialize()
  {
   _backends.clear();

   // Initializing backends
   #ifdef HICR_ENABLE_BACKEND_PTHREADS
	  _backends.push_back(new backends::PThreads());
   #endif

	  _initialized = true;
  }

  /**
   * Retrieves the list of the detected backends.
   *
   * @return A list of detected backends.
   */
  inline std::vector<Backend*>& getBackends()
  {
   if (_initialized == false) LOG_ERROR("Attempting to use HiCR without first initializing it.");

   return _backends;
  }

 private:

  /**
   * Stores the set of detected backends
   */
  std::vector<Backend*> _backends;

};

} // namespace HiCR

