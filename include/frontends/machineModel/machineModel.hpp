
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file machineModel.hpp
 * @brief This file implements the machine model class
 * @author S. M Martin
 * @date 16/01/2024
 */

#pragma once

#include <unordered_set>
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/L0/device.hpp>
#include <backends/host/L0/computeResource.hpp>
#include <backends/host/L0/memorySpace.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
#include <backends/ascend/L0/device.hpp>
#endif

#define _HICR_TOPOLOGY_RPC_UNIT_ID 0xF0F0F0F0
#define _HICR_TOPOLOGY_RPC_NAME "HICR_TOPOLOGY_RPC_"

namespace HiCR
{

/**
 * The MachineModel frontend enables the deployment of multi-instance applications, specifying required the topology for each instance.
 */
class MachineModel
{
  public:
 
  /**
   *  This struct hold the information of a detected instance including its topology
   */
  struct detectedInstance_t
  {
    
    /**
     * Pointer to the detected instance
     */ 
    std::shared_ptr<HiCR::L0::Instance> instance;

    /**
     * Detected topology of the given instance
    */
    HiCR::L0::Topology topology;
  };

  /**
   * This struct hold the information of an instance to create, as described by the machine model
   */ 
  struct request_t
  {
    /**
     * Identifier of the task to execute
     */ 
    std::string taskName;

    /**
     * Number of replicas of this instance to create
     */
    size_t replicaCount;

    /**
     * Requested topology for this instance, in HiCR format
     */ 
    HiCR::L0::Topology topology;

    /**
     * Pointer to the assigned instances (one per replica)
     */ 
    std::vector<std::shared_ptr<HiCR::L0::Instance>> instances;
  };

  /**
   * Default constructor for the machine model class
   * 
   * @param[in] instanceManager Specifies the instance manager to detect and create instances and send RPC requests
  */
  MachineModel(HiCR::L1::InstanceManager& instanceManager) : _instanceManager(&instanceManager)
  {

  }

  HiCR::L0::Topology parseTopology(const nlohmann::json& topologyJson)
  {
    // Storage for the HiCR-formatted topology to create
    HiCR::L0::Topology topology;

    // Parsing CPU cores entry
    if (topologyJson.contains("CPU Cores") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'CPU Cores' entry\n");
    if (topologyJson["CPU Cores"].is_number_unsigned() == false) throw std::runtime_error("The instance topology 'CPU Cores' entry is not a positive number\n");
    size_t numCores = topologyJson["CPU Cores"].get<size_t>();

    // Creating list of compute resources (CPU cores / processing units)
    HiCR::L0::Device::computeResourceList_t computeResources;
    for (size_t i = 0; i < numCores; i++) computeResources.insert(std::make_shared<HiCR::backend::host::L0::ComputeResource>());

    // Parsing host RAM size entry
    if (topologyJson.contains("Host RAM Size (Gb)") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'Host RAM Size (Gb)' entry\n");
    if (topologyJson["Host RAM Size (Gb)"].is_number() == false) throw std::runtime_error("The instance topology 'Host RAM Size (Gb)' entry is not a number\n");
    auto hostRamSize = (size_t)std::ceil(topologyJson["Host RAM Size (Gb)"].get<double>() * 1024.0 * 1024.0 * 1024.0);

    // Creating list of memory spaces (only one, with the total host memory)
    HiCR::L0::Device::memorySpaceList_t memorySpaces ( { std::make_shared<HiCR::backend::host::L0::MemorySpace>( hostRamSize ) });

    // Creating the CPU (NUMA Domain) device type with the assigned number of cores
    auto hostDevice = std::make_shared<HiCR::backend::host::L0::Device>(0, computeResources, memorySpaces);

    // Adding host device to the topology
    topology.addDevice(hostDevice);

    #ifdef _HICR_USE_ASCEND_BACKEND_

    // Parsing ascend device list
    if (topologyJson.contains("Ascend Devices") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'Ascend Devices' entry\n");
    if (topologyJson["Ascend Devices"].is_number_unsigned() == false) throw std::runtime_error("The instance topology 'Ascend Devices' entry is not a positive number\n");
    size_t ascendDeviceCount = topologyJson["Ascend Devices"].get<size_t>();

    // Adding requested ascend devices, if any
    for (size_t i = 0; i < ascendDeviceCount; i++) topology.addDevice(std::make_shared<HiCR::backend::ascend::L0::Device>());
    
    #endif

    // Returning the HiCR-formatted topology
    return topology;
  }

  bool isTopologySubset(const HiCR::L0::Topology& a, const HiCR::L0::Topology& b)
  {
    // For this example, it suffices that topology B has more or equal:
    //  + Total Core Count (among all NUMA domains)
    //  + Total RAM size (among all NUMA domains)
    //  + Ascend devices
    // than topology A.

    size_t tACoreCount = 0;
    size_t tBCoreCount = 0;

    size_t tAMemSize = 0;
    size_t tBMemSize = 0;

    size_t tAAscendDeviceCount = 0;
    size_t tBAscendDeviceCount = 0;

    // Processing topology A
    for (const auto& d: a.getDevices()) 
    {
      const auto deviceType = d->getType();

      // If it's a NUMA Domain device, then its about host requirements
      if (deviceType == "NUMA Domain")
      {
        // Casting to a host device
        auto hostDev = std::dynamic_pointer_cast<HiCR::backend::host::L0::Device>(d);

        // Adding corresponding counts
        tACoreCount += hostDev->getComputeResourceList().size();
        tAMemSize += (*hostDev->getMemorySpaceList().begin())->getSize();
      }

      // It it's an Ascend device, increment the ascend device counter
      if (deviceType == "Ascend Device") tAAscendDeviceCount++;
    } 

    // Processing topology B
    for (const auto& d: b.getDevices()) 
    {
      const auto deviceType = d->getType();

      // If it's a NUMA Domain device, then its about host requirements
      if (deviceType == "NUMA Domain")
      {
        // Casting to a host device
        auto hostDev = std::dynamic_pointer_cast<HiCR::backend::host::L0::Device>(d);

        // Adding corresponding counts
        tBCoreCount += hostDev->getComputeResourceList().size();
        tBMemSize += (*hostDev->getMemorySpaceList().begin())->getSize();
      }

      // It it's an Ascend device, increment the ascend device counter
      if (deviceType == "Ascend Device") tBAscendDeviceCount++;
    } 

    // Evaluating criteria
    if (tACoreCount > tBCoreCount) return false;
    if (tAMemSize   > tBMemSize) return false;
    if (tAAscendDeviceCount > tBAscendDeviceCount) return false;

    // If no criteria failed, return true
    return true;
  }

  std::vector<request_t> parseMachineModel(const nlohmann::json& machineModelJson)
  {
    // Storage for the parsed instances from the machine model
    std::vector<request_t> requests;  

    // Checking for correct format in the machine model
    if (machineModelJson.contains("Instances") == false) throw std::runtime_error("the machine model does not contain an 'Instances' entry\n");
    if (machineModelJson["Instances"].is_array() == false) throw std::runtime_error("The 'Instances' entry in the machine model is not an array\n");

    // Now iterating over all requested instances
    for (const auto& instance : machineModelJson["Instances"])
    {
      // Storage for the new requested instance to add
      request_t newRequestedInstance;

      // Parsing task name
      if (instance.contains("Task") == false) throw std::runtime_error("the requested instance does not contain a 'Task' entry\n"); 
      if (instance["Task"].is_string() == false) throw std::runtime_error("The instance 'Task' entry is not a string\n");
      newRequestedInstance.taskName = instance["Task"].get<std::string>();

      // Parsing replica count
      if (instance.contains("Replicas") == false) throw std::runtime_error("the requested instance does not contain a 'Replicas' entry\n");
      if (instance["Replicas"].is_number_unsigned() == false) throw std::runtime_error("The instance 'Replicas' entry is not an unsigned number\n");
      newRequestedInstance.replicaCount = instance["Replicas"].get<size_t>();

      // Parsing requested topology
      if (instance.contains("Topology") == false) throw std::runtime_error("the requested instance does not contain a 'Topology' entry\n");
      if (instance["Topology"].is_object() == false) throw std::runtime_error("The instance 'Topology' entry is not an object\n");
      newRequestedInstance.topology = parseTopology(instance["Topology"]);

      // Adding newly requested instance to the collection
      requests.push_back(newRequestedInstance);
    }

    // Returning parsed requested instances
    return requests;
  }

  std::vector<detectedInstance_t> detectInstances(HiCR::L1::InstanceManager &instanceManager)
  {
    // Storage for the detected instances
    std::vector<detectedInstance_t> detectedInstances;
    
    // Getting the pointer to our own (coordinator) instance
    auto coordinator = instanceManager.getCurrentInstance();

    // Obtaining each of the instances HiCR topology
    for (const auto &instance : instanceManager.getInstances())
    if (instance->getId() != coordinator->getId())
    {
      // Storage for the newly detected instance
      detectedInstance_t detectedInstance;

      // Assigning instance pointer
      detectedInstance.instance = instance;

      // Running the RPC that obtains the instance's serialized topology
      instanceManager.execute(*instance, _HICR_TOPOLOGY_RPC_NAME);

      // Gathering the return value
      auto returnValue = instanceManager.getReturnValue(*instance);

      // Receiving raw serialized topology information from the worker
      std::string serializedTopology = (char *)returnValue->getPointer();

      // Parsing serialized raw topology into a json object
      auto topologyJson = nlohmann::json::parse(serializedTopology);

      // Obtaining the host topology from the serialized object
      detectedInstance.topology.merge(HiCR::backend::host::hwloc::L1::TopologyManager::deserializeTopology(topologyJson));

      // Obtaining the ascend topology (if enabled at compilation time)
      #ifdef _HICR_USE_ASCEND_BACKEND_
      detectedInstance.topology.merge(HiCR::backend::ascend::L1::TopologyManager::deserializeTopology(topologyJson));
      #endif // _HICR_USE_ASCEND_BACKEND_

      // Adding detected instance to the storage
      detectedInstances.push_back(detectedInstance);
    }

    // Returning detected instances
    return detectedInstances;
  }

  void executeRequests(std::vector<request_t>& requests)
  {
    // Getting information about the currently launched instances and their topology
    auto detectedInstances = detectInstances(*_instanceManager);

    // Now matching requested instances to actual instances, creating new ones if the detected ones do not satisfy their topology requirements
    for (size_t i = 0; i < requests.size(); i++) 
     for (size_t j = 0; j < requests[i].replicaCount; j++)
      {
        // Flag to store whether the request has been assigned an instances
        bool requestAssigned = false;

        // Try to match the requested instances against one of the detected instances
        for (auto dItr = detectedInstances.begin(); dItr != detectedInstances.end() && requestAssigned == false; dItr++)
        {
          // Check if the detected instance topology satisfied the request do the following
          if (isTopologySubset(requests[i].topology, dItr->topology) == true)
          {
            // Assign the instance to the request
            requests[i].instances.push_back(dItr->instance);

            // Mark request as assigned to continue break the loops
            requestAssigned = true;

            // Remove detected instance from the list so it's not assigned to another request
            detectedInstances.erase(dItr);
          }
        }

        // If the request replica was assigned, continue with the next one
        if (requestAssigned == true) continue;
      
        // If no remaining detected instances satisfied the request, then try to create a new instance ad hoc
        auto newInstance = _instanceManager->createInstance(requests[i].topology);

        // Adding new instance to the detected instance set
        if (newInstance != nullptr) { requests[i].instances.push_back(newInstance); requestAssigned = true; }

        // If no instances could be created, then abort
        if (requestAssigned == false)
        {
          std::string errorMsg = std::string("Could not assign nor create an instance for request ") + std::to_string(i) + std::string(", replica ") + std::to_string(j);
          throw std::runtime_error(errorMsg.c_str()); 
        }
      }
  }

  
  __USED__ inline void submitTopology()
  {
    // Fetching memory manager
    auto memoryManager = _instanceManager->getMemoryManager();
    
    // Getting current instance
    auto currentInstance = _instanceManager->getCurrentInstance();

    // Storage for the topology to send
    HiCR::L0::Topology workerTopology;

    // List of topology managers to query
    std::vector<HiCR::L1::TopologyManager*> topologyManagerList;

    // Now instantiating topology managers (which ones is determined by backend availability during compilation)
    #ifdef _HICR_USE_HWLOC_BACKEND_

    // Creating HWloc topology object
    hwloc_topology_t topology;

    // Reserving memory for hwloc
    hwloc_topology_init(&topology);

    // Initializing HWLoc-based host (CPU) topology manager
    HiCR::backend::host::hwloc::L1::TopologyManager hwlocTopologyManager(&topology);

    // Adding topology manager to the list
    topologyManagerList.push_back(&hwlocTopologyManager);

    #endif // _HICR_USE_HWLOC_BACKEND_

    #ifdef _HICR_USE_ASCEND_BACKEND_

    // Initialize (Ascend's) ACL runtime
    aclError err = aclInit(NULL);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

    // Initializing ascend topology manager
    HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;

    // Adding topology manager to the list
    topologyManagerList.push_back(&ascendTopologyManager);

    #endif // _HICR_USE_ASCEND_BACKEND_

    // For each topology manager detected
    for (const auto& tm : topologyManagerList)
    {
      // Getting the topology information from the topology manager
      const auto t = tm->queryTopology();

      // Merging its information to the worker topology object to send
      workerTopology.merge(t);
    } 

    // Serializing theworker topology and dumping it into a raw string message
    auto message = workerTopology.serialize().dump();

    // Registering memory slot at the first available memory space as source buffer to send the return value from
    auto sendBuffer = memoryManager->registerLocalMemorySlot(_instanceManager->getBufferMemorySpace(), message.data(), message.size() + 1);

    // Registering return value
    _instanceManager->submitReturnValue(sendBuffer);

    // Deregistering memory slot
    memoryManager->deregisterLocalMemorySlot(sendBuffer);
  };


  private:

  HiCR::L1::InstanceManager* const _instanceManager;
};

} // namespace HiCR
