#pragma once

#include "common.hpp"
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>

// Worker task functions
void taskFc(const std::string &taskName, HiCR::L1::InstanceManager &instanceManager)
{
  // Getting current instance
  auto currentInstance = instanceManager.getCurrentInstance();

  // Serializing theworker topology and dumping it into a raw string message
  auto message = std::string("Hello, I am worker ") + std::to_string(currentInstance->getId()) + std::string(" executing Task: ") + taskName;

  // Registering return value
  instanceManager.submitReturnValue(message.data(), message.size() + 1);
};

void workerFc(HiCR::L1::InstanceManager &instanceManager, std::vector<HiCR::L1::TopologyManager *> &topologyManagers)
{
  // Flag to indicate whether the worker should continue listening
  bool continueListening = true;

  // Creating machine model to handle the instance creation and task execution
  HiCR::MachineModel machineModel(instanceManager, topologyManagers);

  // Adding RPC targets, specifying a name and the execution unit to execute
  instanceManager.addRPCTarget("Finalize", [&]() { continueListening = false; });
  instanceManager.addRPCTarget("Task A", [&]() { taskFc("A", instanceManager); });
  instanceManager.addRPCTarget("Task B", [&]() { taskFc("B", instanceManager); });
  instanceManager.addRPCTarget("Task C", [&]() { taskFc("C", instanceManager); });

  // Listening for RPC requests
  while (continueListening == true) instanceManager.listen();
}
