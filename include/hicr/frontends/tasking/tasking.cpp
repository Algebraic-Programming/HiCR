#include <pthread.h>
#include <hicr/frontends/tasking/tasking.hpp>

namespace HiCR
{
namespace tasking
{
/**
     * A thread-specific pointer that stored the currently running HiCR Tasking Worker pointer
     */
pthread_key_t _workerPointerKey;

/**
     * A thread-specific pointer that stored the currently running HiCR Tasking Task pointer
     */
pthread_key_t _taskPointerKey;

/**
     * Indicates whether the tasking sub-system was initialized
     */
bool _isInitialized = false;
} // namespace tasking
} // namespace HiCR
