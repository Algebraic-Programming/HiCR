/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of (multi-threaded) shared memory systems
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <backends/sequential/L0/executionUnit.hpp>
#include <backends/sequential/L1/computeManager.hpp>
#include <backends/sharedMemory/hwloc/L0/computeResource.hpp>
#include <backends/sharedMemory/pthreads/L0/processingUnit.hpp>
#include <hicr/L1/computeManager.hpp>
#include <memory>

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
class ComputeManager final : public sequential::L1::ComputeManager
{
  public:

  /**
   * Constructor for the compute manager class for the shared memory backend
   */
  ComputeManager() : sequential::L1::ComputeManager() {}
  ~ComputeManager() = default;

  private:

  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnitImpl(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    return std::make_unique<sharedMemory::pthreads::L0::ProcessingUnit>(computeResource);
  }
};

} // namespace L1

} // namespace pthreads

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
