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

#include <vector>
#include <memory>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>

#define BUFFER_SIZE 256
#define ITERATIONS 3
#define DST_OFFSET 0
#define SRC_OFFSET 0

void telephoneGame(HiCR::MemoryManager                            &m,
                   HiCR::CommunicationManager                     &c,
                   std::shared_ptr<HiCR::LocalMemorySlot>          input,
                   std::vector<std::shared_ptr<HiCR::MemorySpace>> memSpaces,
                   int                                                 iterations)
{
  // Collect the newly created memory slots
  auto memSlots = std::vector<std::shared_ptr<HiCR::LocalMemorySlot>>{};

  // iterate all over the memory spaces and create multiple memory slots in each one
  for (const auto &memSpace : memSpaces)
    for (int i = 0; i < iterations; i++) memSlots.emplace_back(m.allocateLocalMemorySlot(memSpace, BUFFER_SIZE));

  // Getting input memory slot
  auto srcMemSlot = input;

  // For each of the detected memory slots...
  for (auto dstMemSlot : memSlots)
  {
    // Initialize the memory slot; smoke test for memset
    m.memset(dstMemSlot, 0, BUFFER_SIZE);

    // Perform the memcpy operations
    c.memcpy(dstMemSlot, DST_OFFSET, srcMemSlot, SRC_OFFSET, BUFFER_SIZE);

    // fence when the memcpy happens between two different memory spaces
    c.fence(0);

    // update source memory slot
    srcMemSlot = dstMemSlot;
  }

  // Getting output memory slot (the last one in the list)
  auto output = *memSlots.rbegin();

  // print the output of the telephone game
  printf("Input: %s\n", (const char *)input->getPointer());
  printf("Output: %s\n", (const char *)output->getPointer());

  // free the memory slots
  for (auto &memSlot : memSlots) m.freeLocalMemorySlot(memSlot);
}
