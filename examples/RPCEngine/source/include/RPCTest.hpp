#pragma once

#include <hicr/core/L1/computeManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>

void RPCTestFc(HiCR::L1::CommunicationManager &cm,
               HiCR::L1::MemoryManager &mm,
               HiCR::L1::ComputeManager &cpm, 
               HiCR::L1::InstanceManager &im, 
               std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace, 
               std::shared_ptr<HiCR::L0::ComputeResource> computeResource,
               std::shared_ptr<HiCR::L0::ExecutionUnit> rpcExecutionUnit)
{
  // Creating RPC engine instance
  HiCR::frontend::RPCEngine rpcEngine(cm, im, mm, cpm, bufferMemorySpace, computeResource);

  // Initialize RPC engine
  rpcEngine.initialize();

  // Getting current instance
  auto currentInstance = im.getCurrentInstance();

  // Registering RPC to listen to
  rpcEngine.addRPCTarget("Test RPC", rpcExecutionUnit);

  // If I'm instance zero, I request an RPC.
  if (currentInstance->getId() == 0)
  {
   for (auto& instance : im.getInstances()) 
    if (instance != currentInstance)
     rpcEngine.requestRPC(*instance, "Test RPC");
  }
  else rpcEngine.listen();
}
