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

#include <hicr/backends/sharedMemory/computeManager.hpp>
#include <hicr/backends/sharedMemory/memoryManager.hpp>
#include <hicr/machineModel/deviceModel.hpp>
#include <hicr/machineModel/hostdev/CPU.hpp>

namespace HiCR
{

namespace machineModel
{

class HostDevice final: public DeviceModel
{
  private:
    std::vector<Cache> _sharedCaches; // revisit

  public:
    // New form, TODO remove local allocations in initialize();
    // managers come from the caller (ProcessManager?)
    //HostDevice(
    //   backend::sharedMemory::ComputeManager *compMan,
    //   backend::sharedMemory::MemoryManager *memMan
    //):
    //_computeMan(compMan),
    //_memoryMan(memMan)
    //{
    //   _type = "host";
    //}
    HostDevice()
    {
        _type = "host";
    }

    void initialize() override
   {
       // Creating HWloc topology object
       hwloc_topology_t topology;

       // Reserving memory for hwloc
       hwloc_topology_init(&topology);

       // Initialize Backend-specific Compute & Memory Managers
       _computeMan = new backend::sharedMemory::ComputeManager(&topology);
       _memoryMan  = new backend::sharedMemory::MemoryManager(&topology);

       _computeMan->queryComputeResources();
       _memoryMan->queryMemorySpaces();

       // Populate our own resource representation based on the backend-specific Managers
       for (auto m : _memoryMan->getMemorySpaceList())
       {
           backend::MemoryManager::memorySpaceId_t tmp_id = m;
           std::string tmp_type = "NUMA Domain";
           MemorySpace *ms = new MemorySpace(
                   tmp_id, /* memorySpaceId_t */
                   tmp_type, /* type */
                   _memoryMan->getMemorySpaceSize(tmp_id) /* size */
                   );
           _memorySpaces.insert(std::make_pair(tmp_id, ms));
       }

       for (auto c : _computeMan->getComputeResourceList())
       {
           computeResourceId_t tmp_id = c;
           CPU *cmp = new CPU(
                   tmp_id /* computeResourceId_t */
                   );
           _computeResources.insert(std::make_pair(tmp_id, cmp));
       }

       // NOTE: Since we created the pointers in this same function, it is safe to assume static cast correctness
       backend::sharedMemory::ComputeManager *compMan = static_cast<backend::sharedMemory::ComputeManager *>(_computeMan);
       //backend::sharedMemory::MemoryManager  *memMan  = static_cast<backend::sharedMemory::MemoryManager *>(_memoryMan);

        for (auto com : _computeResources)
        {
            CPU *c = static_cast<CPU *>(com.second);
            auto coreId = c->getId();
            c->setCaches(  compMan->getCpuCaches(coreId));
            c->setSiblings(compMan->getCpuSiblings(coreId));
            c->setSystemId(compMan->getCpuSystemId(coreId));
            auto memspaceId = compMan->getCpuNumaAffinity(coreId);
            c->addMemorySpace(memspaceId);
            auto ms = _memorySpaces.at(memspaceId);
            ms->addComputeResource(coreId);
        }

   }

   void jSerializeImpl(nlohmann::json& json)
   {
       if (json["Device Type"] != "host") // The way it's currently written, this should never happen; placeholder for future check
           HICR_THROW_RUNTIME("Device type incompatibility in JSON creation");

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
               switch(cache.getCacheType())
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
                   sharingPUs = sharingPUs.substr(0, sharingPUs.find_last_not_of(" ") +1);
                   json["ComputeResources"][index]["caches"][cacheType]["sharing PUs"] = sharingPUs;
               }

           }

           json["ComputeResources"][index]["NumaAffinity"] = *cpu->getMemorySpaces().find(0); //just get the 1st element for now

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
           compUnits = compUnits.substr(0, compUnits.find_last_not_of(" ") +1);
           json["MemorySpaces"][ms->getId()]["compute units"] = compUnits;
       } // end of Memory Spaces section
   }

   void shutdown() override 
    {
        for (auto it : _memorySpaces)
            delete it.second;

        for (auto it : _computeResources)
            delete it.second;

        delete(_computeMan);
        delete(_memoryMan);
    }

}; // class HostDevice

} // namespace machineModel


} // namespace HiCR

