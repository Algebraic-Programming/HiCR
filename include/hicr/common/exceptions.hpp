/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file exceptions.hpp
 * @brief Provides a failure model and corresponding exception classes.
 * @author A. N. Yzelman
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
   * \param[in] message Explanation message for this exception
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
   * \param[in] message Explanation message for this exception
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
   * \param[in] message  Explanation message for this exception
   */
  __USED__ FatalException(const char *const message) : runtime_error(message) {}
};

} // end namespace HiCR
