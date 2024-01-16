#pragma once

#include <unordered_set>
#include "common.hpp"
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/L0/device.hpp>
#include <backends/host/L0/computeResource.hpp>
#include <backends/host/L0/memorySpace.hpp>
#include <frontends/machineModel/machineModel.hpp>

void finalizeExecution(HiCR::L1::InstanceManager &instanceManager, const int returnCode = 0)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Requesting workers to abort and printing error message 
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, "Finalize");

  HiCR::finalize();
  exit(returnCode);
}

void coordinatorFc(HiCR::L1::InstanceManager &instanceManager, const std::string& machineModelFilePath)
{
  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFilePath);
  if (status == false) {  fprintf(stderr, "could not read from machine model file: '%s'\n", machineModelFilePath.c_str()); finalizeExecution(instanceManager, -1); }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try { machineModelJson = nlohmann::json::parse(machineModelRaw); }
   catch (const std::exception& e)
    {  fprintf(stderr, "could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFilePath.c_str(), e.what()); finalizeExecution(instanceManager, -1); }

  // Creating machine model to handle the instance creation and task execution
  HiCR::MachineModel machineModel(instanceManager);

  // Parsing the machine model into a request vector. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
  std::vector<HiCR::MachineModel::request_t> requests;
  try { requests = machineModel.parseMachineModel(machineModelJson); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());  finalizeExecution(instanceManager, -1); }

  // Execute requests by finding or creating an instance that matches their topology requirements
  try { machineModel.executeRequests(requests); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());  finalizeExecution(instanceManager, -1); }

  // Running the assigned task id in the correspondng instance
  for (auto &r : requests)
   for (auto &in : r.instances)
    instanceManager.execute(*in, r.taskName);

  // Now waiting for return values to arrive
  for (auto &r : requests)
   for (auto &in : r.instances)
    {
      // Getting return value as a memory slot
      auto returnValue = instanceManager.getReturnValue(*in);

      // Printing return value
      printf("[Coordinator] Received from instance %lu: '%s'\n", in->getId(), (const char*)returnValue->getPointer());
    }

  // Finalizing execution for all instances
  finalizeExecution(instanceManager);
}
