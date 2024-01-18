#pragma once

#include <unordered_set>
#include "common.hpp"
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <frontends/machineModel/machineModel.hpp>

void finalizeExecution(std::shared_ptr<HiCR::L1::InstanceManager> instanceManager, const int returnCode = 0)
{
  // Querying instance list
  auto &instances = instanceManager->getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager->getCurrentInstance();

  // Requesting workers to abort and printing error message
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager->launchRPC(*instance, "Finalize");

  HiCR::finalize();
  exit(returnCode);
}

/*
 * This function allows us to customize the criteria that determine whether the detected topology satisfies one of our requests
 */
bool isTopologyAcceptable(const HiCR::L0::Topology &a, const HiCR::L0::Topology &b)
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
  for (const auto &d : a.getDevices())
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
  for (const auto &d : b.getDevices())
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
  if (tAMemSize > tBMemSize) return false;
  if (tAAscendDeviceCount > tBAscendDeviceCount) return false;

  // If no criteria failed, return true
  return true;
}

void coordinatorFc(std::shared_ptr<HiCR::L1::InstanceManager> instanceManager, const std::string &machineModelFilePath)
{
  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager->getCurrentInstance();

  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFilePath);
  if (status == false)
  {
    fprintf(stderr, "could not read from machine model file: '%s'\n", machineModelFilePath.c_str());
    finalizeExecution(instanceManager, -1);
  }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try
  {
    machineModelJson = nlohmann::json::parse(machineModelRaw);
  }
  catch (const std::exception &e)
  {
    fprintf(stderr, "could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFilePath.c_str(), e.what());
    finalizeExecution(instanceManager, -1);
  }

  // Creating machine model to handle the instance creation and task execution
  HiCR::MachineModel machineModel(instanceManager);

  // Parsing the machine model into a request vector. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
  std::vector<HiCR::MachineModel::request_t> requests;
  try
  {
    requests = machineModel.parse(machineModelJson);
  }
  catch (const std::exception &e)
  {
    fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());
    finalizeExecution(instanceManager, -1);
  }

  // Execute requests by finding or creating an instance that matches their topology requirements
  try
  {
    machineModel.deploy(requests, &isTopologyAcceptable);
  }
  catch (const std::exception &e)
  {
    fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());
    finalizeExecution(instanceManager, -1);
  }

  // Running the assigned task id in the correspondng instance
  for (auto &r : requests)
    for (auto &in : r.instances)
      instanceManager->launchRPC(*in, r.taskName);

  // Now waiting for return values to arrive
  for (auto &r : requests)
    for (auto &in : r.instances)
    {
      // Getting return value as a memory slot
      auto returnValue = instanceManager->getReturnValue(*in);

      // Printing return value
      printf("[Coordinator] Received from instance %lu: '%s'\n", in->getId(), (const char *)returnValue->getPointer());
    }

  // Finalizing execution for all instances
  finalizeExecution(instanceManager);
}
