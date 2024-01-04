/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file exceptions.hpp
 * @brief Provides a failure model and corresponding exception classes.
 * @author A. N. Yzelman & Sergio Martin
 * @date 9/8/2023
 */

#pragma once

#include <cstdlib>
#include <stdarg.h>
#include <stdexcept>
#include <hicr/definitions.hpp>

namespace HiCR
{

namespace exceptions
{

/**
 * Enumeration of different exception types in HiCR for internal use
 */
enum exception_t
{
  /**
   * Represents a logic exception
   */
  logic,

  /**
   * Represents a runtime exception
   */
  runtime,

  /**
   * Represents a fatal exception
   */
  fatal
};

} // namespace exceptions

/**
 * Macro for throwing a logic exception in HiCR. It includes additional information in the message, such as line number and source file.
 *
 * \param[in] ... C-Formatted string and its additional arguments
 */
#define HICR_THROW_LOGIC(...) [[unlikely]] HiCR::throwException(HiCR::exceptions::exception_t::logic, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Macro for throwing a runtime exception in HiCR. It automatically includes additional information in the message, such as line number and source file.
 *
 * \param[in] ... C-Formatted string and its additional arguments
 */
#define HICR_THROW_RUNTIME(...) [[unlikely]] HiCR::throwException(HiCR::exceptions::exception_t::runtime, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Macro for throwing a fatal exception in HiCR. It automatically includes additional information in the message, such as line number and source file.
 *
 * \param[in] ... C-Formatted string and its additional arguments
 */
#define HICR_THROW_FATAL(...) [[unlikely]] HiCR::throwException(HiCR::exceptions::exception_t::fatal, __FILE__, __LINE__, __VA_ARGS__)

/**
 * A class of exceptions that indicate some error in the arguments to a HiCR
 * call.
 *
 * When an exception of this (super)type is thrown, it shall be as though the
 * call to the HiCR primitive that threw it, had never been made.
 *
 * \note This is also known as \em no-side-effect error handling.
 *
 * \internal This puts more pressure on the developers of HiCR, but is likely
 *           a very useful guarantee for HiCR users.
 */
class LogicException : public std::logic_error
{
  public:

  /**
   * Constructor for a logic exception
   *
   * @param[in] message Explanation message for this exception
   */
  __USED__ LogicException(const char *const message) : logic_error(message) {}
};

/**
 * A class of exceptions that indicate a non-fatal runtime error has been
 * encountered during a call to a HiCR primitive.
 *
 * When an exception of this (super)type is thrown, it shall be as though the
 * call to the HiCR primitive that threw it, had never been made.
 *
 * \note This is also known as \em no-side-effect error handling.
 *
 * \internal This puts more pressure on the developers of HiCR, but is likely
 *           a very useful guarantee for HiCR users.
 */
class RuntimeException : public std::runtime_error
{
  public:

  /**
   * Constructor for a runtime exception
   *
   * @param[in] message Explanation message for this exception
   */
  __USED__ RuntimeException(const char *const message) : runtime_error(message) {}
};

/**
 * A class of exceptions that are fatal in that the HiCR runtime enters some
 * undefined state. When this class of exceptions are caught, users can only
 * attempt to wind down the application as gracefully as possible, without
 * calling any other HiCR functionality.
 */
class FatalException : public std::runtime_error
{
  public:

  /**
   * Constructor for a fatal exception
   *
   * @param[in] message  Explanation message for this exception
   */
  __USED__ FatalException(const char *const message) : runtime_error(message) {}
};

/**
 * This function creates the exception message for a HiCR exception (for internal use)
 *
 * @param[in] type Inovked exception type
 * @param[in] fileName The source file where this exception has been thrown
 * @param[in] lineNumber Line number inside the source file where this exception has been thrown
 * @param[in] format C-Formatted message provided by the user explaining the reason of the exception
 * @param[in] ... Arguments, if any, to the C-Formatted message
 */
__USED__ inline void throwException [[noreturn]] (const exceptions::exception_t type, const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  auto res = vasprintf(&outstr, format, ap);

  if (res < 0)
  {
    auto errorMsg = std::string("Error in exceptions.hpp, throwLogic() function\n");

#ifdef HICR_EXCEPTION_USE_STDEXCEPTION
    throw std::runtime_error(errorMsg.c_str());
#endif

#ifdef HICR_EXCEPTION_USE_ABORT
    fprintf(stderr, "%s", errorMsg.c_str());
    fflush(stderr);
    std::abort();
#endif
  }

  std::string typeString = "Undefined";
  switch (type)
  {
  case exceptions::exception_t::logic: typeString = "Logic"; break;
  case exceptions::exception_t::runtime: typeString = "Runtime"; break;
  case exceptions::exception_t::fatal: typeString = "Fatal"; break;
  default: break;
  }
  std::string outString = std::string("[HiCR] ") + typeString + std::string(" Exception: ") + std::string(outstr);
  free(outstr);

  char info[1024];
  snprintf(info, sizeof(info) - 1, " + From %s:%d\n", fileName, lineNumber);
  outString += info;

#ifdef HICR_EXCEPTION_USE_STDEXCEPTION

  switch (type)
  {
  case exceptions::exception_t::logic: throw LogicException(outString.c_str()); break;
  case exceptions::exception_t::runtime: throw RuntimeException(outString.c_str()); break;
  case exceptions::exception_t::fatal: throw FatalException(outString.c_str()); break;
  default: break;
  }

  throw std::runtime_error(outString.c_str());

#endif

// #ifdef HICR_EXCEPTION_USE_ABORT

  fprintf(stderr, "%s", outString.c_str());
  fflush(stderr);
  std::abort();

// #endif
}

} // namespace HiCR
