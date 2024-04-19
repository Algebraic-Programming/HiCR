
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
#include <nlohmann_json/json.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L1/topologyManager.hpp>

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
     * Identifier of the entry point to execute
     */
    std::string entryPointName;

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
   * @param[in] topologyManagers Specifies the topology managers to employ for topology detection and serialization
   */
  MachineModel(HiCR::L1::InstanceManager &instanceManager, std::vector<HiCR::L1::TopologyManager *> &topologyManagers)
    : _instanceManager(&instanceManager),
      _topologyManagers(topologyManagers)
  {
    // Registering Topology parsing function as callable RPC
    _instanceManager->addRPCTarget(_HICR_TOPOLOGY_RPC_NAME, [&]() { submitTopology(&instanceManager, _topologyManagers); });
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
   * @param[in] argc argc to pass to the newly created instance
   * @param[in] argv argv to pass to the newly created instance
   */
  void deploy(std::vector<request_t> &requests, topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc = 0, char *argv[] = nullptr)
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
        std::shared_ptr<HiCR::L0::Instance> newInstance;
        try
        {
          newInstance = _instanceManager->createInstance(requests[i].topology, argc, argv);
        }
        catch (std::exception &e)
        {
          HICR_THROW_RUNTIME("Tried to create new instances while deploying to satisfy the machine model requests, but an error ocurred: '%s'", e.what());
        }

        // Adding new instance to the detected instance set
        requests[i].instances.push_back(newInstance);
        requestAssigned = true;
      }

    // Checking no excess of instances were created/detected
    if (detectedInstances.empty() == false) HICR_THROW_LOGIC("An excess number of instances were detected after deploying the machine model.");
  }

  private:

  std::vector<detectedInstance_t> detectInstances(HiCR::L1::InstanceManager &instanceManager)
  {
    // Storage for the detected instances
    std::vector<detectedInstance_t> detectedInstances;

    // Getting the pointer and adding the local (currentInstance) instance first
    detectedInstance_t currentInstance;
    currentInstance.instance = instanceManager.getCurrentInstance();
    currentInstance.topology.merge(getTopology(_topologyManagers));
    detectedInstances.push_back(currentInstance);

    // Obtaining each of the other detectable instances' HiCR topology
    for (const auto &instance : instanceManager.getInstances())
      if (instance->getId() != instanceManager.getCurrentInstance()->getId())
      {
        // Storage for this detected instance
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

        // Obtaining topology from the registered topology managers
        for (const auto tm : _topologyManagers) detectedInstance.topology.merge(tm->_deserializeTopology(topologyJson));

        // Adding detected instance to the storage
        detectedInstances.push_back(detectedInstance);
      }

    // Returning detected instances
    return detectedInstances;
  }

  __INLINE__ static void submitTopology(HiCR::L1::InstanceManager *instanceManager, std::vector<HiCR::L1::TopologyManager *> &topologyManagers)
  {
    // Storage for the topology to send
    auto workerTopology = getTopology(topologyManagers);

    // For each topology manager detected
    for (const auto tm : topologyManagers)
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

  private:

  __INLINE__ static HiCR::L0::Topology getTopology(std::vector<HiCR::L1::TopologyManager *> &topologyManagers)
  {
    // Storage for the topology to send
    HiCR::L0::Topology topology;

    // For each topology manager detected
    for (const auto tm : topologyManagers)
    {
      // Getting the topology information from the topology manager
      const auto t = tm->queryTopology();

      // Merging its information to the worker topology object to send
      topology.merge(t);
    }

    return topology;
  }

  /**
   * Instance manager to use for instance detection
   */
  HiCR::L1::InstanceManager *_instanceManager;

  /**
   * Topology managers to use for resource detection
   */
  std::vector<HiCR::L1::TopologyManager *> _topologyManagers;
};

} // namespace HiCR
