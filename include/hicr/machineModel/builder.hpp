
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file machineModel/builder.hpp
 * @brief Definition of the HiCR machine model Builder class
 * @author O. Korakitis & S. M. Martin
 * @date 24/11/2023
 */

#pragma once

#include <hicr/instance.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/machineModel/model.hpp>
#include <hicr/backends/sequential/computeManager.hpp>
#include <hicr/backends/instanceManager.hpp>

/**
 * Internal id of the processing unit id to use when running the machine model worker RPC
 */
#define HICR_MACHINE_MODEL_RPC_PROCESSING_UNIT_ID 4096

/**
 * Internal id of the execution unit id to use when running the machine model worker RPC
 */
#define HICR_MACHINE_MODEL_RPC_EXECUTION_UNIT_ID 4096

namespace HiCR
{

namespace machineModel
{

/**
 * Class definition for the HiCR machine model builder.
 *
 * This class provides an interface for building the machine model from all visible HiCR instances
 *
 * It requires an instance manager for reaching out to other HiCR instances.
 */
class Builder
{
  public:

 /**
  * Constructor for the machine model Builder class. It requires the passing of an instance manager
  *
  * @param[in] instanceManager The instance manager for the discovery of other instances
  */
  Builder(backend::InstanceManager *instanceManager) :
   _instanceManager(instanceManager)
  {
  }

  ~Builder() = default;

  /**
   * This function creates a unified machine model from all the intervening instances
   *
   * @param[in] rootInstanceId Specifies the instance to receive the complete model
   */
  __USED__ inline void build(const Instance::instanceId_t rootInstanceId)
  {
    // Getting current instance pointer
    auto currentInstance = _instanceManager->getCurrentInstance();

    // Getting current instance id
    const auto currentInstanceId = currentInstance->getId();

    // If the current instance is not root, then listen for incoming RPCs and return
    if (currentInstanceId != rootInstanceId) { workerFunction(currentInstance); return; }

    // Calling coordinator function, in charge of putting back together the machine model
    coordinatorFunction(currentInstance);
  }

  /**
   * This function serialized the instance internal models into a string that can be printed to screen or log
   *
   * @return A stringified representation of the internal state of the machine model
   */
  __USED__ inline std::string stringify() const
  {
   std::string output;

   // Attaching all instance model strings into a single string
   for (const auto& entry : _instanceModelMap)
   {
    // Getting instance id
    const auto instanceId = entry.first;

    // Getting stringified model
    const auto modelString = entry.second.stringify();

    // Creating new entry
    output += std::string("HiCR Instance ") + std::to_string(instanceId) + std::string(" Model: \n") + modelString + std::string("\n");
   }

   return output;
  }

  private:

  /**
   * Internal function for the machine model builder coordinator to run
   *
   * @param[in] currentInstance A pointer to the current (coordinator) instance
   */
  __USED__ inline void coordinatorFunction(HiCR::Instance* currentInstance)
  {
   // Clearing current instance/model map
   _instanceModelMap.clear();

   // Querying instance list
   auto& instances = _instanceManager->getInstances();

   // Printing instance information and invoking a simple RPC if its not ourselves
   for (const auto& instance : instances) if (instance != currentInstance)
     instance->execute(HICR_MACHINE_MODEL_RPC_PROCESSING_UNIT_ID, HICR_MACHINE_MODEL_RPC_EXECUTION_UNIT_ID);

   // Pushing coordinators own local machine model
   machineModel::Model coordinatorMachineModel;

   // Updating local model
   coordinatorMachineModel.update();

   // Adding to the instance machine model collection
   _instanceModelMap[currentInstance->getId()] = coordinatorMachineModel;

   // Getting machine models from other instances
   for (const auto& instance : instances) if (instance != currentInstance)
   {
    // Getting serialized machine model information from the instance
    const auto returnValue = instance->getReturnValue();

    // Converting returned value to string for subsequent parsing
    const auto instanceModelString = std::string((const char*)returnValue->getPointer(), returnValue->getSize());

    // Deserializing model
    machineModel::Model instanceModel(instanceModelString);

    // Pushing instance into the vector
    _instanceModelMap[instance->getId()] = instanceModel;
   }
  }

  /**
   * Internal function for the machine model builder worker to run
   *
   * @param[in] currentInstance A pointer to the current (worker) instance
   */
  __USED__ inline void workerFunction(HiCR::Instance* currentInstance)
  {
   // Initializing sequential backend
   HiCR::backend::sequential::ComputeManager computeManager;

   // Fetching memory manager
   auto memoryManager = _instanceManager->getMemoryManager();

   // Creating worker function
   auto fcLambda = [currentInstance, memoryManager]()
   {
    // Creating local machine model and updating its resources
    machineModel::Model localModel;

    // Updating (detecting) the local machine model
    localModel.update();

    // Serializing machine model to send it to the coordinator instance
    auto message = localModel.serialize();

    // Registering memory slot at the first available memory space as source buffer to send the return value from
    auto sendBuffer = memoryManager->registerLocalMemorySlot(message.data(), message.size());

    // Registering return value
    currentInstance->submitReturnValue(sendBuffer);

    // Deregistering memory slot
    memoryManager->deregisterLocalMemorySlot(sendBuffer);
   };

   // Creating execution unit
   auto executionUnit = computeManager.createExecutionUnit(fcLambda);

   // Querying compute resources
   computeManager.queryComputeResources();

   // Getting compute resources
   auto computeResources = computeManager.getComputeResourceList();

   // Creating processing unit from the compute resource
   auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());

   // Initialize processing unit
   processingUnit->initialize();

   // Assigning processing unit to the instance manager
   currentInstance->addProcessingUnit(HICR_MACHINE_MODEL_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

   // Assigning processing unit to the instance manager
   currentInstance->addExecutionUnit(HICR_MACHINE_MODEL_RPC_EXECUTION_UNIT_ID, executionUnit);

   // Listening for RPC requests
   currentInstance->listen();
  }

  /**
   * Pointer to the backend that is in charge of managing instances, rpcs and their return values
   */
  backend::InstanceManager *const _instanceManager;

  /**
   * This map links instance ids to their machine model
   */
  std::map<Instance::instanceId_t, machineModel::Model> _instanceModelMap;
};

} // namespace machineModel

} // namespace HiCR
