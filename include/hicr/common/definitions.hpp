#pragma once

namespace HiCR
{

// API annotation for inline functions, to make sure they are considered during coverage analysis
// This solution was taken from https://stackoverflow.com/a/10824832
#if defined __GNUC__ && defined ENABLE_COVERAGE
  #define HICR_API __attribute__((__used__))
#else
  #define HICR_API
#endif

} // namespace HiCR
