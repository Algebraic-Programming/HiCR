
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
#include <functional>
#include <vector>
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/L0/device.hpp>
#include <backends/host/L0/computeResource.hpp>
#include <backends/host/L0/memorySpace.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <backends/ascend/L0/device.hpp>
#endif

/**
 * Execution unit id for the predetermined topology-exchange RPC
 */
#define _HICR_TOPOLOGY_RPC_EXECUTION_UNIT_ID 0xF0F0F0F0

/**
 * Internal name of the predetermined topology-exchange RPC
 */
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
   * Function type for the evaluation topology acceptance criteria (whether an instance topology satisfies a given request)
   */
  typedef std::function<bool(const HiCR::L0::Topology &, const HiCR::L0::Topology &)> topologyAcceptanceCriteriaFc_t;

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
    std::vector<std::shared_ptr<HiCR::L0::Instance>> instances = {};
  };

  /**
   * Default constructor for the machine model class
   *
   * @param[in] instanceManager Specifies the instance manager to detect and create instances and send RPC requests
   */
  MachineModel(HiCR::L1::InstanceManager &instanceManager) : _instanceManager(&instanceManager)
  {
    // Registering Topology parsing function as callable RPC
    _instanceManager->addRPCTarget(_HICR_TOPOLOGY_RPC_NAME, [&]()
                                   { submitTopology(&instanceManager); });
  }

  /**
   * This function deploys the requested machine model into the available system resources.
   * It receives a set of machine requests and uses the instance manager to resolve whether:
   * - A yet-unasigned instance exists that can satisfy the given request or, otherwise
   * - A new instance can be created with the minimal set of hardware resources to satisfy that request
   * This function will fail (exception) if neither of the two conditions above can be met
   *
   * @param[in] requests A vector of machine requests. The requests will be resolved in the order provided.
   * @param[in] acceptanceCriteriaFc A function that determines, given the detect topology and the requested topology, if the former satisfies the latter
   */
  void deploy(std::vector<request_t> &requests, topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc)
  {
    // Getting information about the currently deployed instances and their topology
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
          if (acceptanceCriteriaFc(requests[i].topology, dItr->topology) == true)
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
        requests[i].instances.push_back(newInstance);
        requestAssigned = true;
      }
  }

  private:

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
        instanceManager.launchRPC(*instance, _HICR_TOPOLOGY_RPC_NAME);

        // Gathering the return value
        auto returnValue = instanceManager.getReturnValue(*instance);

        // Receiving raw serialized topology information from the worker
        std::string serializedTopology = (char *)returnValue;

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

  __USED__ static inline void submitTopology(HiCR::L1::InstanceManager *instanceManager)
  {
    // Storage for the topology to send
    HiCR::L0::Topology workerTopology;

    // List of topology managers to query
    std::vector<HiCR::L1::TopologyManager *> topologyManagerList;

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
    for (const auto &tm : topologyManagerList)
    {
      // Getting the topology information from the topology manager
      const auto t = tm->queryTopology();

      // Merging its information to the worker topology object to send
      workerTopology.merge(t);
    }

    // Serializing theworker topology and dumping it into a raw string message
    auto message = workerTopology.serialize().dump();

    // Registering return value
    instanceManager->submitReturnValue(message.data(), message.size() + 1);
  };

  HiCR::L1::InstanceManager *_instanceManager;
};

} // namespace HiCR
