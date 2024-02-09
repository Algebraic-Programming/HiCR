/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file channel.hpp
 * @brief Provides a generic definition for a worker RPC channel
 * @author S. M. Martin
 * @date 7/2/2024
 */

#include <frontends/channel/variableSize/spsc/producer.hpp>

#pragma once

namespace HiCR
{

namespace runtime
{

/**
 * Defines the a class that enables unidirectional, variable sized token communication between runtime instances
 *
 * Although the API is common, the implementation is specialized to each given backend
 */
class Channel final
{
  public:

  Channel() = default;
  ~Channel() = default;

  private:
};

} // namespace runtime

} // namespace HiCR
