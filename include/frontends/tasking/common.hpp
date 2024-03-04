/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief This file implements variables shared among mutiple code files
 * @author Sergio Martin
 * @date 3/4/2024
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <pthread.h>

namespace HiCR
{

namespace tasking
{

/**
 * Key identifier for thread-local identification of currently running task
 */
static pthread_key_t _taskPointerKey;

/**
 * Key identifier for thread-local identification of currently running worker
 */
static pthread_key_t _workerPointerKey;

/**
 * Flag to indicate whether the tasking subsystem was initialized
 */
static bool _isInitialized = false;

} // namespace tasking

} // namespace HiCR
