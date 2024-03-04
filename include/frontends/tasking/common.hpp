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

namespace HiCR
{

namespace tasking
{

/**
 * Flag to indicate whether the tasking subsystem was initialized
 */
static bool _isInitialized = false;

} // namespace tasking

} // namespace HiCR
