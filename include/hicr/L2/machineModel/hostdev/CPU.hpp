/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file CPU.hpp
 * @brief Defines the class and API for interacting with CPUs as a Compute Resource
 * @author O. Korakitis
 * @date 15/11/2023
 */
#pragma once
#include <vector>

#include <hicr/L2/machineModel/computeResource.hpp>
#include <hicr/L2/machineModel/hostdev/cache.hpp>

namespace HiCR
{

namespace machineModel
{

/**
 * A ComputeResource class representing a CPU, as found in common
 * multiprocessor systems. Instances of the class are expected to be the
 * "leaves" in a multicore hierarchy: the logical thread, if Hyperthreading
 * is enabled, or the core, if no Hyperthreading occurs.
 */
class CPU : public ComputeResource
{
  private:

  /**
   *
   * The ID of the hardware core; in SMT systems that will mean the core ID,
   * which may also have other HW threads. In non SMT systems it is expected
   * for logical and system IDs to be 1-to-1.
   */
  unsigned int _systemId;

  /**
   * List of sibling threads/cores, if applicable.
   */
  std::vector<unsigned int> _siblings;

  /**
   * List of Cache objects associated with the CPU. There is the assumption
   * that only one cache object of each type can be associated with a CPU.
   */
  std::vector<Cache> _caches;

  public:

  /**
   * Parametric constructor of the CPU object
   *
   * \param[in] id Identifier of the compute resource
   */
  CPU(HiCR::L0::computeResourceId_t id) : ComputeResource(id, "Core")
  {
  }

  /**
   * Obtain the Cache of a given type associated with the current CPU.
   *
   * \param[in] t A Cache::cacheType identifier
   * \return A Cache object of the given type
   */
  Cache getCache(Cache::cacheType t) const
  {
    for (auto c : _caches)
    {
      if (c.getCacheType() == t)
        return c;
    }

    HICR_THROW_RUNTIME("Cache object of the requested level not found");
    Cache ret;
    return ret;
  }

  /**
   * Alternative (to getCache(type) method to obtain all Caches associated
   * with the current CPU.
   *
   * \return A vector of Cache objects
   */
  std::vector<Cache> getAllCaches() const
  {
    return _caches;
  }

  /**
   * (Redundant) method to check if a cache level is shared or private
   *
   * \param[in] t A Cache::cacheType identifier
   * \return True, if the cache is shared; False if private.
   */
  inline bool isCacheShared(Cache::cacheType t) const
  {
    bool ret = false;
    for (auto c : _caches)
    {
      if (c.getCacheType() == t)
        ret = c.isShared();
    }
    return ret;
  }

  /**
   * Set the CPU's detected caches, as returned by the backend.
   * In order to have the (L0) backend stay completely agnostic to the current (L1)
   * representation -therefore not include any class/structure from L1 in L0- the communication
   * occurs through standard containers and strings.
   *
   * As a result, this function is mostly just parsing the strings and assigning values to Cache object fields
   * This should be used only during initialization / resource detection.
   *
   * \param[in] input A vector of entries, each one an std::pair<type, size>, encapsulating the size (in Bytes)
   *            and type information for each cache. The 'type' string is expected to have (strictly) the following form:
   *            "Level <I/D/U> <P/S> <associated IDs>", where:
   *            Level: may be "L1", "L2, "L3"
   *            I/D/U: may be "Instruction", "Data", "Unified"
   *            P/S:   may be "Private" or "Shared"
   *            associated IDs: (optional, for Shared cache) a list of core IDs, e.g. "0 1 2 3"
   *
   */
  inline void setCaches(std::vector<std::pair<std::string, size_t>> input)
  {
    _caches.resize(input.size());
    size_t count = 0;

    for (auto i : input)
    {
      std::string type = i.first;
      size_t size = i.second;

      // Parse cache Level and type; For L2/L3 assume Unified for now.
      if (type.starts_with("L1"))
      {
        if (type.find("Instruction") != type.npos)
          _caches[count].setCacheType(Cache::L1i);
        else                                       // if (type.find("Data") != type.npos)
          _caches[count].setCacheType(Cache::L1d); // If not specified, assume Data
      }
      else if (type.starts_with("L2"))
        _caches[count].setCacheType(Cache::L2);
      else if (type.starts_with("L3"))
        _caches[count].setCacheType(Cache::L3);

      // Parse sharing status and associated CPUs
      if (type.find("Private") != type.npos)
        _caches[count].setAssociatedComputeUnit(_id);

      else if (type.find("Shared") != type.npos)
      {
        /* This is more easily read from inside-to-outside:
         * We expect the processor numbers to follow the "Shared" token (e.g. "L3 Unified Shared 0 1 2 3")
         * 1. Find the "Shared" word starting position
         * 2. Get the first whitespace *after* the word
         * 3. Get the first non-space after the space in (2), which should be the start of the substring we want.
         */
        std::string puIDs = type.substr(/* Get the last part containing the PU IDs */
                                        type.find_first_not_of(" " /* Detect first non-space char after "Shared" */,
                                                               type.find_first_of(" ", type.find("Shared"))),
                                        type.size() /* Get till the end of the type string */
        );
        while (!puIDs.empty())
        {
          size_t end = puIDs.find_first_of(" ", 0);
          int tmpId = std::stoi(puIDs.substr(0, end));
          _caches[count].addAssociatedComputeUnit(tmpId);
          if (puIDs.find_first_not_of(" ", end) != puIDs.npos)
            puIDs = puIDs.substr(puIDs.find_first_not_of(" ", end), puIDs.size());
          else
            puIDs = ""; // instead of break...
        }
      }

      _caches[count].setCacheSize(size);
      _caches[count].setLineSize(); // TODO: properly query it in the backend

      ++count;
    }
  }

  /**
   * Obtains the Core ID of the CPU; in non SMT systems that will be the actual id;
   * in SMT it is the id of the actual core the thread belongs to.
   *
   * \return The System ID of the hardware Core
   */
  inline unsigned int getSystemId() const
  {
    return _systemId;
  }

  /**
   * Set the System ID
   * This should be used only during initialization / resource detection.
   *
   * \param[in] id The id of the actual core the thread belongs to
   */
  inline void setSystemId(unsigned int id)
  {
    _systemId = id;
  }

  /**
   * Obtains the sibling threads of the CPU, if any. That will include e.g. threads
   * from the same hardware core, sharing L1/L2 etc. In non SMT systems there should
   * be no siblings in that sense.
   *
   * \return A (potentially empty) vector of Resource IDs
   */
  inline std::vector<unsigned int> getSiblings() const
  {
    // Copy
    return _siblings;
  }

  /**
   * Assign the siblings of a CPU. That will include e.g. threads from
   * the same hardware core, sharing L1/L2 etc. In non SMT systems there should
   * be no siblings in that sense.
   * This should be used only during initialization / resource detection.
   *
   * \param[in] siblings A (potentially empty) vector of resource IDs corresponding to the
   *            relevant CPU objects
   */
  inline void setSiblings(std::vector<unsigned int> siblings)
  {
    _siblings = siblings;
  }
}; // class CPU

} // namespace machineModel

} // namespace HiCR
