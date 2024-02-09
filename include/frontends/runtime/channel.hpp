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

#pragma once

#include <frontends/channel/variableSize/spsc/producer.hpp>

#ifdef _HICR_USE_MPI_BACKEND_
  #include <backends/mpi/L1/instanceManager.hpp>
  #include <backends/mpi/L1/communicationManager.hpp>
  #include <backends/mpi/L1/memoryManager.hpp>
  #include <frontends/channel/variableSize/spsc/consumer.hpp>
  #include <frontends/channel/variableSize/spsc/producer.hpp>

  #define _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY 1048576
  #define _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY 1024
  #define _HICR_RUNTIME_CHANNEL_BASE_TAG 0xF0000000
  #define _HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG                      _HICR_RUNTIME_CHANNEL_BASE_TAG + 0
  #define _HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG                    _HICR_RUNTIME_CHANNEL_BASE_TAG + 1
  #define _HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG         _HICR_RUNTIME_CHANNEL_BASE_TAG + 3
  #define _HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG      _HICR_RUNTIME_CHANNEL_BASE_TAG + 4
  #define _HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG    _HICR_RUNTIME_CHANNEL_BASE_TAG + 5
  #define _HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 6
#endif

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
