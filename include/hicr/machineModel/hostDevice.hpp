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

