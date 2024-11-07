/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file implements the pthread-based compute manager for host (CPU) backends
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/backends/host/L0/executionUnit.hpp>
#include <hicr/backends/host/L0/computeResource.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "../L0/processingUnit.hpp"

namespace HiCR::backend::host::pthreads::L1
{

/**
 * Implementation of the pthread-based HiCR Shared Memory backend's compute manager.
 */
class ComputeManager : public HiCR::backend::host::L1::ComputeManager
{
  public:

  ComputeManager()
    : HiCR::backend::host::L1::ComputeManager()
  {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() override = default;

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    return std::make_unique<host::pthreads::L0::ProcessingUnit>(computeResource);
  }
};

} // namespace HiCR::backend::host::pthreads::L1
