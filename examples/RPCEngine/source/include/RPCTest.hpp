/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <hicr/core/computeManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>

void RPCTestFc(HiCR::CommunicationManager            &cm,
               HiCR::MemoryManager                   &mm,
               HiCR::ComputeManager                  &cpm,
               HiCR::InstanceManager                 &im,
               std::shared_ptr<HiCR::MemorySpace>     bufferMemorySpace,
               std::shared_ptr<HiCR::ComputeResource> computeResource,
               std::shared_ptr<HiCR::ExecutionUnit>   rpcExecutionUnit)
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
  if (currentInstance->isRootInstance())
  {
    for (auto &instance : im.getInstances())
      if (instance != currentInstance) rpcEngine.requestRPC(*instance, "Test RPC");
  }
  else
    rpcEngine.listen();
}
