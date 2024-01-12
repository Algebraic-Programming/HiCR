#pragma once

#include <unordered_set>
#include "common.hpp"
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>

// This struct hold the information of an instance to create, as described by the machine model
struct instance_t
{
  // Task name to execute
  std::string taskName;

  // Number of replicas of this instance to create
  size_t replicaCount;

  // Requested topology for this instance, in HiCR format
  HiCR::L0::Topology topology;
};

void abortExecution(HiCR::L1::InstanceManager &instanceManager)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Requesting workers to abort and printing error message 
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, ABORT_EXECUTION_UNIT_ID);

  HiCR::finalize();
  exit(-1);
}

HiCR::L0::Topology parseTopology(const nlohmann::json& topologyJson)
{
  HiCR::L0::Topology topology;

  return topology;
}

std::unordered_set<std::shared_ptr<instance_t>> parseMachineModel(const nlohmann::json& machineModelJson)
{
  // Storage for the parsed instances from the machine model
  std::unordered_set<std::shared_ptr<instance_t>> requestedInstances;  

  // Checking for correct format in the machine model
  if (machineModelJson.contains("Instances") == false) {  fprintf(stderr, "Error: the machine model does not contain an 'Instances' entry\n"); abortExecution(instanceManager);}
  if (machineModelJson["Instances"].is_array() == false) {  fprintf(stderr, "Error: The 'Instances' entry in the machine model is not an array\n"); abortExecution(instanceManager);}

  // Now iterating over all requested instances
  for (const auto& instance : machineModelJson["Instances"])
  {
    // Storage for the new requested instance to add
    auto newRequestedInstance = std::make_shared<instance_t>();

    // Parsing task name
    if (instance.contains("Task") == false) {  fprintf(stderr, "Error: the requested instance does not contain a 'Task' entry\n"); abortExecution(instanceManager);}
    if (instance["Task"].is_string() == false) {  fprintf(stderr, "Error: The instance 'Task' entry is not a string\n"); abortExecution(instanceManager);}
    newRequestedInstance->taskName = instance["Task"].get<std::string>();

    // Parsing replica count
    if (instance.contains("Replicas") == false) { fprintf(stderr, "Error: the requested instance does not contain a 'Replicas' entry\n"); abortExecution(instanceManager);}
    if (instance["Replicas"].is_number_unsigned() == false) { fprintf(stderr, "Error: The instance 'Replicas' entry is not an unsigned number\n"); abortExecution(instanceManager);}
    newRequestedInstance->replicaCount = instance["Replicas"].get<size_t>();

    // Parsing requested topology
    if (instance.contains("Topology") == false) { fprintf(stderr, "Error: the requested instance does not contain a 'Topology' entry\n"); abortExecution(instanceManager);}
    if (instance["Topology"].is_object() == false) { fprintf(stderr, "Error: The instance 'Topology' entry is not an object\n"); abortExecution(instanceManager);}
    newRequestedInstance->topology = parseTopology(instance["Topology"]);

    // Adding newly requested instance to the collection
    requestedInstances.insert(newRequestedInstance);
  }

  // Returning parsed requested instances
  return requestedInstances;
}

void coordinatorFc(HiCR::L1::InstanceManager &instanceManager, const std::string& machineModelFilePath)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFilePath);
  if (status == false) {  fprintf(stderr, "Error: could not read from machine model file: '%s'\n", machineModelFilePath.c_str()); abortExecution(instanceManager); }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try { machineModelJson = nlohmann::json::parse(machineModelRaw); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error: could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFilePath.c_str(), e.what()); abortExecution(instanceManager); }

  // Parsing the machine model into its requested instances
  auto requestedInstances = parseMachineModel(machineModelJson);

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, TASK_A_EXECUTION_UNIT_ID);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances) if (instance != coordinator)
  {
    // Getting return value as a memory slot
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Printing return value
    printf("[Coordinator] Received from instance %lu: '%s'\n", instance->getId(), (const char*)returnValue->getPointer());
  }
}
