/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file hostDevice.hpp
 * @brief Defines the API for interacting with the host (CPUs) device type
 * @author O. Korakitis
 * @date 05/10/2023
 */
#pragma once

#include <backends/sharedMemory/L1/computeManager.hpp>
#include <backends/sharedMemory/L1/memoryManager.hpp>
#include <hicr/L2/machineModel/deviceModel.hpp>
#include <hicr/L2/machineModel/hostdev/CPU.hpp>

namespace HiCR
{

namespace L2
{

namespace machineModel
{

/**
 * Device model pertaining to a host (CPU) type device
 */
class HostDevice final : public DeviceModel
{
  private:

  std::vector<Cache> _sharedCaches; // revisit

  public:

  // New form, TODO remove local allocations in initialize();
  // managers come from the caller (ProcessManager?)
  // HostDevice(
  //   backend::sharedMemory::ComputeManager *compMan,
  //   backend::sharedMemory::MemoryManager *memMan
  //):
  //_computeManager(compMan),
  //_memoryManager(memMan)
  //{
  //   _type = "host";
  //}
  HostDevice()
  {
    _type = "host";
  }

  /**
   *  Constructor through JSON for serialized; to be used to instantiate remote devices
   *
   *  @param[in] json The JSON encoded information about this host device
   */
  HostDevice(nlohmann::json json)
  {
    // Not local so these should not be used!
    _computeManager = nullptr;
    _memoryManager = nullptr;

    size_t computeCount = (size_t)json["ComputeResources"]["NumComputeRes"];
    size_t memCount = (size_t)json["NumMemSpaces"];
    for (size_t i = 0; i < memCount; i++)
    {
      if (json["MemorySpaces"][i]["type"] != "NUMA Domain")
        HICR_THROW_RUNTIME("Potential misconfiguration: Not matching NUMA domain in MemorySpace type");

      std::string tmp_type = "NUMA Domain";
      size_t size = (size_t)json["MemorySpaces"][i]["size"];
      MemorySpace *ms = new MemorySpace(
        i,        /* memorySpaceId_t */
        tmp_type, /* type */
        size      /* size */
      );
      _memorySpaces.insert(std::make_pair(i, ms));
    }

    for (size_t i = 0; i < computeCount; i++)
    {
      CPU *c = new CPU(i);

      std::string index = "Core " + std::to_string(i);

      c->setSystemId(json["ComputeResources"][index]["systemCoreId"]);

      // Detect siblings from the string; parse the string one by one and assign
      std::string strSiblings = json["ComputeResources"][index]["siblings"];
      std::vector<unsigned> cpuSiblings;
      strSiblings = strSiblings.substr(0, strSiblings.find_first_not_of(" "));
      while (!strSiblings.empty())
      {
        std::string id = strSiblings.substr(0, strSiblings.find_first_of(" "));
        cpuSiblings.push_back(std::stoi(id));
        strSiblings = strSiblings.substr(strSiblings.find_first_of(" ") + 1, strSiblings.size() - 1);
      }
      c->setSiblings(cpuSiblings);

      _computeResources.insert(std::make_pair(i, c));

      // Detect caches and create strings' vector compatible with the setCaches() method:
      std::vector<std::string> cachetypes = {"L1i", "L1d", "L2", "L3"};

      std::vector<std::pair<std::string, size_t>> strCaches;

      strCaches.resize(4); // Hardcoded for 1st prototype FIXME
      for (auto type : cachetypes)
      {
        std::string tmp;
        if (type == "L1i")
          tmp = "L1 Instruction";
        else if (type == "L1d")
          tmp = "L1 Data";
        else if (type == "L2")
          tmp = "L2 Unified";
        else if (type == "L3")
          tmp = "L3 Unified";

        tmp += " ";

        if (json["ComputeResources"][index]["caches"][type]["shared"])
        {
          tmp += "Shared";
          tmp += " ";
          tmp += json["ComputeResources"][index]["caches"][type]["sharing PUs"];
        }
        else
          tmp += "Private";

        size_t cachesize = json["ComputeResources"][index]["caches"][type]["size"];

        strCaches.push_back(std::make_pair(tmp, cachesize));
      }

      // Assign the newly created object:
      c->setCaches(strCaches);
    }
  }

  void initialize() override
  {
    // Creating HWloc topology object
    hwloc_topology_t topology;

    // Reserving memory for hwloc
    hwloc_topology_init(&topology);

    // Initialize Backend-specific Compute & Memory Managers
    _computeManager = new backend::sharedMemory::L1::ComputeManager(&topology);
    _memoryManager = new backend::sharedMemory::L1::MemoryManager(&topology);

    _computeManager->queryComputeResources();
    _memoryManager->queryMemorySpaces();

    // Populate our own resource representation based on the backend-specific Managers
    for (auto m : _memoryManager->getMemorySpaceList())
    {
      L1::MemoryManager::memorySpaceId_t tmp_id = m;
      std::string tmp_type = "NUMA Domain";
      MemorySpace *ms = new MemorySpace(
        tmp_id,                                    /* memorySpaceId_t */
        tmp_type,                                  /* type */
        _memoryManager->getMemorySpaceSize(tmp_id) /* size */
      );
      _memorySpaces.insert(std::make_pair(tmp_id, ms));
    }

    for (auto c : _computeManager->getComputeResourceList())
    {
      HiCR::L0::computeResourceId_t tmp_id = c;
      CPU *cmp = new CPU(
        tmp_id /* HiCR::L0::computeResourceId_t */
      );
      _computeResources.insert(std::make_pair(tmp_id, cmp));
    }

    // NOTE: Since we created the pointers in this same function, it is safe to assume static cast correctness
    backend::sharedMemory::L1::ComputeManager *compMan = static_cast<backend::sharedMemory::L1::ComputeManager *>(_computeManager);
    // backend::sharedMemory::MemoryManager  *memMan  = static_cast<backend::sharedMemory::MemoryManager *>(_memoryManager);

    for (auto com : _computeResources)
    {
      CPU *c = static_cast<CPU *>(com.second);
      auto coreId = c->getId();
      c->setCaches(compMan->getCpuCaches(coreId));
      c->setSiblings(compMan->getCpuSiblings(coreId));
      c->setSystemId(compMan->getCpuSystemId(coreId));
      auto memspaceId = compMan->getCpuNumaAffinity(coreId);
      c->addMemorySpace(memspaceId);
      auto ms = _memorySpaces.at(memspaceId);
      ms->addComputeResource(coreId);
    }
  }

  void jSerializeImpl(nlohmann::json &json)
  {
    //       if (json["Device Type"] != "host") // The way it's currently written, this should never happen; placeholder for future check
    //           HICR_THROW_RUNTIME("Device type incompatibility in JSON creation");

    // Compute Resources section
    json["ComputeResources"]["NumComputeRes"] = getComputeCount();
    for (auto c : _computeResources)
    {
      CPU *cpu = static_cast<CPU *>(c.second);
      std::string index = "Core " + std::to_string(c.first);
      std::string siblings;
      for (auto id : cpu->getSiblings())
        siblings += std::to_string(id) + " ";
      size_t end = siblings.find_last_not_of(" ") + 1;
      json["ComputeResources"][index]["siblings"] = siblings.substr(0, end);

      json["ComputeResources"][index]["systemCoreId"] = cpu->getSystemId();

      for (auto cache : cpu->getAllCaches())
      {
        std::string cacheType;
        switch (cache.getCacheType())
        {
        case Cache::L1i:
          cacheType = "L1i";
          break;
        case Cache::L1d:
          cacheType = "L1d";
          break;
        case Cache::L2:
          cacheType = "L2";
          break;
        case Cache::L3:
          cacheType = "L3";
          break;
        }

        json["ComputeResources"][index]["caches"][cacheType]["size"] = cache.getCacheSize();
        json["ComputeResources"][index]["caches"][cacheType]["linesize"] = cache.getLineSize();
        json["ComputeResources"][index]["caches"][cacheType]["shared"] = cache.isShared();
        if (cache.isShared())
        {
          std::string sharingPUs;
          for (auto id : cache.getAssociatedComputeUnit())
            sharingPUs += std::to_string(id) + " ";
          sharingPUs = sharingPUs.substr(0, sharingPUs.find_last_not_of(" ") + 1);
          json["ComputeResources"][index]["caches"][cacheType]["sharing PUs"] = sharingPUs;
        }
      }

      json["ComputeResources"][index]["NumaAffinity"] = *cpu->getMemorySpaces().find(0); // just get the 1st element for now

    } // end of Compute Resources section

    // Memory Spaces section
    json["NumMemSpaces"] = _memorySpaces.size();
    for (auto memspace : _memorySpaces)
    {
      MemorySpace *ms = memspace.second;

      json["MemorySpaces"][ms->getId()]["type"] = ms->getType();
      json["MemorySpaces"][ms->getId()]["size"] = ms->getSize();
      std::string compUnits;
      for (auto id : ms->getComputeUnits())
        compUnits += std::to_string(id) + " ";
      compUnits = compUnits.substr(0, compUnits.find_last_not_of(" ") + 1);
      json["MemorySpaces"][ms->getId()]["compute units"] = compUnits;
    } // end of Memory Spaces section
  }

  void shutdown() override
  {
    for (auto it : _memorySpaces)
      delete it.second;

    for (auto it : _computeResources)
      delete it.second;

    delete (_computeManager);
    delete (_memoryManager);
  }

}; // class HostDevice

} // namespace machineModel

} // namespace L2

} // namespace HiCR
