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

#include <hicr/common/definitions.hpp>

namespace HiCR
{

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

// Macros for exception throwing
#define THROW_LOGIC(...)   HiCR::throwException(exception_t::logic,   __FILE__, __LINE__, __VA_ARGS__)
#define THROW_RUNTIME(...) HiCR::throwException(exception_t::runtime, __FILE__, __LINE__, __VA_ARGS__)
#define THROW_FATAL(...)   HiCR::throwException(exception_t::fatal,   __FILE__, __LINE__, __VA_ARGS__)
__USED__ inline void throwException [[noreturn]] (const exception_t type, const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  auto res = vasprintf(&outstr, format, ap);
  if (res < 0) throw std::runtime_error("Error in exceptions.hpp, throwLogic() function\n");

  std::string typeString = "Undefined";
  switch(type)
  {
   case exception_t::logic:   typeString = "Logic"; break;
   case exception_t::runtime: typeString = "Runtime"; break;
   case exception_t::fatal:   typeString = "Fatal"; break;
   default: break;
  }
  std::string outString = std::string("[HiCR] ") + typeString + std::string(" Exception: ") + std::string(outstr);
  free(outstr);

  char info[1024];
  snprintf(info, sizeof(info) - 1, " + From %s:%d\n", fileName, lineNumber);
  outString += info;

  switch(type)
  {
   case exception_t::logic:   throw LogicException(outString.c_str()); break;
   case exception_t::runtime: throw RuntimeException(outString.c_str()); break;
   case exception_t::fatal:   throw FatalException(outString.c_str()); break;
   default: break;
  }

  throw std::runtime_error(outString.c_str());
}


} // end namespace HiCR
