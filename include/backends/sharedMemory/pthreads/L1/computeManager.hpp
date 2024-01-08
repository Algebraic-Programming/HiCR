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
#include <backends/sharedMemory/L0/executionUnit.hpp>
#include <backends/sharedMemory/L0/computeResource.hpp>
#include <backends/sharedMemory/L1/computeManager.hpp>
#include <backends/sharedMemory/pthreads/L0/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace pthreads
{

namespace L1
{

/**
 * Implementation of the pthread-based HiCR Shared Memory backend's compute manager.
 */
class ComputeManager : public HiCR::backend::sharedMemory::L1::ComputeManager
{
  public:

  ComputeManager() : HiCR::backend::sharedMemory::L1::ComputeManager() {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() = default;

  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const
  {
    return std::make_unique<sharedMemory::pthreads::L0::ProcessingUnit>(computeResource);
  }
};

} // namespace L1

} // namespace pthreads

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
