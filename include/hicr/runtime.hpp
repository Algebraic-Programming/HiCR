#pragma once

#include <memory>

#include <hicr/backend.hpp>
#include <hicr/common/logger.hpp>

#ifdef HICR_ENABLE_BACKEND_PTHREADS
  #include <hicr/backends/pthreads.hpp>
#endif

namespace HiCR
{

typedef std::vector<std::unique_ptr<Backend>> backendList_t;

/**
 * @brief Main HiCR runtime system class
 *
 * It can be used to query the system's available resources and their connectivity.
 */
class Runtime
{
  public:
  /**
   * Detects the backends selected by the user at compilation time and stores them internally.
   */
  void initialize()
  {
    _backends.clear();

// Initializing backends
#ifdef HICR_ENABLE_BACKEND_PTHREADS
    _backends.push_back(std::make_unique<backends::PThreads>());
#endif
  }

  /**
   * Retrieves the list of backends detected at initialization.
   *
   * @return A list of the detected backends.
   */
  inline const backendList_t &getBackends() const
  {
    return _backends;
  }

  private:
  /**
   * Storage for the detected backends
   */
  backendList_t _backends;
};

} // namespace HiCR
