#pragma once

#include "common.hpp"
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>


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

  // Checking for correct format in the machine model
  if (machineModelJson.contains("Instances") == false) {  fprintf(stderr, "Error: the machine model does not contain an 'Instances' entry\n"); abortExecution(instanceManager);}
  if (machineModelJson["Instances"].is_array() == false) {  fprintf(stderr, "Error: The 'Instances' entry in the machine model is not an array\n"); abortExecution(instanceManager);}

  // Now iterating over all requested instances
  for (const auto& instance : machineModelJson["Instances"])
  {
    if (instance.contains("Task") == false) {  fprintf(stderr, "Error: the reuested instance does not contain a 'Task' entry\n"); abortExecution(instanceManager);}
    if (instance["Task"].is_string() == false) {  fprintf(stderr, "Error: The instance 'Task' entry is not a string\n"); abortExecution(instanceManager);}

    if (instance.contains("Replicas") == false) { fprintf(stderr, "Error: the reuested instance does not contain a 'Replicas' entry\n"); abortExecution(instanceManager);}
    if (instance["Replicas"].is_number_unsigned() == false) { fprintf(stderr, "Error: The instance 'Replicas' entry is not an unsigned number\n"); abortExecution(instanceManager);}

    if (instance.contains("Topology") == false) { fprintf(stderr, "Error: the rqeuested instance does not contain a 'Topology' entry\n"); abortExecution(instanceManager);}
    if (instance["Topology"].is_object() == false) { fprintf(stderr, "Error: The instance 'Topology' entry is not an object\n"); abortExecution(instanceManager);}
  }

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
