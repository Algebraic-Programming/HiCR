#pragma once

#include <hicr/common/definitions.hpp>
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

namespace HiCR
{

// Error logging function
#define LOG_ERROR(...) HiCR::logError(__FILE__, __LINE__, __VA_ARGS__)
__USED__ inline void logError [[noreturn]] (const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  auto res = vasprintf(&outstr, format, ap);
  if (res < 0) throw std::runtime_error("Error in logError\n");

  std::string outString = std::string("[Error] ") + std::string(outstr);
  free(outstr);

  char info[1024];

  snprintf(info, sizeof(info) - 1, " + From %s:%d\n", fileName, lineNumber);
  outString += info;

  throw std::runtime_error(outString.c_str());
}

// warning logging function
#define LOG_WARNING(...) HiCR::logWarning(__FILE__, __LINE__, __VA_ARGS__)
__USED__ inline void logWarning(const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  auto res = vasprintf(&outstr, format, ap);
  if (res < 0) throw std::runtime_error("Error in logWarning\n");

  std::string outString = std::string("[Warning] ") + std::string(outstr);
  free(outstr);

  char info[1024];

  snprintf(info, sizeof(info) - 1, " + From %s:%d\n", fileName, lineNumber);
  outString += info;

  fprintf(stderr, "%s", outString.c_str());
}

// warning logging function
#define LOG_DEBUG(...) HiCR::logDebug(__FILE__, __LINE__, __VA_ARGS__)
__USED__ inline void logDebug(const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  auto res = vasprintf(&outstr, format, ap);
  if (res < 0) throw std::runtime_error("Error in logDebug\n");

  std::string outString = std::string("[Debug] ") + std::string(outstr);
  free(outstr);

  fprintf(stderr, "%s\n", outString.c_str());
}

} // namespace HiCR
