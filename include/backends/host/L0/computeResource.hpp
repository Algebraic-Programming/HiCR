/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file core.hpp
 * @brief This file implements the abstract compute resource class for the host (CPU) backends
 * @author O. Korakitis & S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <memory>
#include <unordered_set>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L0/computeResource.hpp>
#include <backends/host/cache.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace L0
{

/**
 * This class represents a compute resource, visible by the host backend.
 * That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class ComputeResource : public HiCR::L0::ComputeResource
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
   * \param[in] physicalProcessorId The identifier of the physical core as assigned by the OS
   */
  ComputeResource(const logicalProcessorId_t                                       logicalProcessorId,
                  const physicalProcessorId_t                                      physicalProcessorId,
                  const numaAffinity_t                                             numaAffinity,
                  const std::unordered_set<std::shared_ptr<backend::host::Cache>> &caches)
    : HiCR::L0::ComputeResource(),
      _logicalProcessorId(logicalProcessorId),
      _physicalProcessorId(physicalProcessorId),
      _numaAffinity(numaAffinity),
      _caches(caches){};
  ~ComputeResource() = default;

  /**
   * Default constructor for serialization/deserialization purposes
   */
  ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "Processing Unit"; }

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

  protected:

  __USED__ inline void serializeImpl(nlohmann::json &output) const override
  {
    // Writing core's information into the serialized object
    output["Logical Processor Id"]  = _logicalProcessorId;
    output["Physical Processor Id"] = _physicalProcessorId;
    output["NUMA Affinity"]         = _numaAffinity;

    // Writing Cache information
    std::string cachesKey = "Caches";
    output[cachesKey]     = std::vector<nlohmann::json>();
    for (const auto &cache : _caches) output[cachesKey] += cache->serialize();
  }

  __USED__ inline void deserializeImpl(const nlohmann::json &input) override
  {
    std::string key = "Logical Processor Id";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _logicalProcessorId = input[key].get<logicalProcessorId_t>();

    key = "Physical Processor Id";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _physicalProcessorId = input[key].get<physicalProcessorId_t>();

    key = "NUMA Affinity";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _numaAffinity = input[key].get<numaAffinity_t>();

    key = "Caches";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_array() == false) HICR_THROW_LOGIC("The '%s' entry is not an array", key.c_str());

    _caches.clear();
    for (const auto &c : input[key])
    {
      // Deserializing cache
      auto cache = std::make_shared<backend::host::Cache>(c);

      // Adding it to the list
      _caches.insert(cache);
    }
  }

  private:

  /**
   * The logical ID of the hardware core / processing unit
   */
  logicalProcessorId_t _logicalProcessorId;

  /**
   * The ID of the hardware core; in SMT systems that will mean the core ID,
   * which may also have other HW threads. In non SMT systems it is expected
   * for logical and system IDs to be 1-to-1.
   */
  physicalProcessorId_t _physicalProcessorId;

  /**
   * The ID of the hardware NUMA domain that this core is associated to
   */
  numaAffinity_t _numaAffinity;

  /**
   * List of Cache objects associated with the CPU. There is the assumption
   * that only one cache object of each type can be associated with a CPU.
   */
  std::unordered_set<std::shared_ptr<backend::host::Cache>> _caches;
};

} // namespace L0

} // namespace host

} // namespace backend

} // namespace HiCR
