/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file core.hpp
 * @brief This file implements the compute resource class for the shared memory backend
 * @author O. Korakitis & S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L0/computeResource.hpp>
#include <backends/sharedMemory/cache.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class Core : public HiCR::L0::ComputeResource
{
  public:

  /**
   * System-given logical processor (core or hyperthread) identifier that this class instance represents
   */
  typedef int logicalProcessorId_t;

  /**
   * System-given physical processor identifier that this class instance represents
   */
  typedef int physicalProcessorId_t;

  /**
   * System-given NUMA affinity identifier
   */
  typedef int numaAffinity_t;

  /**
   * Constructor for the compute resource class of the sequential backend
   * \param[in] logicalProcessorId Unique identifier for the core assigned to this compute resource
   * \param[in] numaAffinity The NUMA domain associated to this core
   * \param[in] caches The set of caches contained to or accessible by this core
   * \param[in] siblings The set of other cores local to this core (either in the same socket or NUMA domain)
   * \param[in] physicalProcessorId The identifier of the physical core as assigned by the OS
   */
  Core(const logicalProcessorId_t logicalProcessorId,
       const physicalProcessorId_t physicalProcessorId,
       const numaAffinity_t numaAffinity,
       const std::vector<backend::sharedMemory::Cache> &caches,
       const std::vector<logicalProcessorId_t> &siblings) : HiCR::L0::ComputeResource(),
                                                            _logicalProcessorId(logicalProcessorId),
                                                            _physicalProcessorId(physicalProcessorId),
                                                            _numaAffinity(numaAffinity),
                                                            _caches(caches),
                                                            _siblings(siblings){};
  Core() = delete;

  /**
   * Default destructor
   */
  ~Core() = default;

  __USED__ inline std::string getType() const override { return "CPU Core"; }

  /**
   * Function to return the compute resource processor id
   *
   * @returns The processor id
   */
  __USED__ inline int getProcessorId() const { return _logicalProcessorId; }

  /**
   * Obtains the Core ID of the CPU; in non SMT systems that will be the actual id;
   * in SMT it is the id of the actual core the thread belongs to.
   *
   * \return The physical ID of the hardware Core
   */
  __USED__ inline physicalProcessorId_t getPhysicalProcessorId() const { return _physicalProcessorId; }

  private:

  /**
   * The logical ID of the hardware core / processing unit
   */
  const logicalProcessorId_t _logicalProcessorId;

  /**
   * The ID of the hardware core; in SMT systems that will mean the core ID,
   * which may also have other HW threads. In non SMT systems it is expected
   * for logical and system IDs to be 1-to-1.
   */
  const physicalProcessorId_t _physicalProcessorId;

  /**
   * The ID of the hardware NUMA domain that this core is associated to
   */
  const numaAffinity_t _numaAffinity;

  /**
   * List of Cache objects associated with the CPU. There is the assumption
   * that only one cache object of each type can be associated with a CPU.
   */
  const std::vector<backend::sharedMemory::Cache> _caches;

  /**
   * List of sibling threads/cores, if applicable.
   */
  const std::vector<logicalProcessorId_t> _siblings;
};

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
