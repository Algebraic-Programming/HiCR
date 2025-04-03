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

/**
 * @file dataObject.hpp
 * @brief Provides functionality for a data object over HiCR
 * @author A. N. Yzelman, O. Korakitis
 * @date 29/11/2024
 */

#pragma once

#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>
#include <hicr/core/L0/instance.hpp>

namespace HiCR::objectStore
{

/**
 * The type of a block ID.
 */
using blockId = uint32_t;

/**
 * The struct used to form a handle to a block, with the intent of copying it
 * locally or transmitting it to other workers.
 * All fields are trivially copyable, hence the struct is trivially copyable,
 * as it does not contain user-defined or other methods.
 */
struct serializedMetadata_t
{
  /**
   * The instance ID of the block's owner.
   */
  L0::Instance::instanceId_t instanceId;

  /**
   * The ID of the block.
   */
  blockId id;

  /**
   * The size of the block.
   */
  size_t size;

  /**
   * The global memory slot of the block in serialized form.
   */
  uint8_t serializedGlobalSlot[28 + sizeof(size_t)]; // TODO fix hard-coded size
};

/**
 * The type of a handle to a DataObject.
 */
using handle = serializedMetadata_t;

// Ensure the struct is trivially copyable
static_assert(std::is_trivially_copyable<serializedMetadata_t>::value, "DataObject handle must be trivially copyable");

/**
 * Encapsulates the concept of a block in the object store.
 */
class DataObject
{
  friend class ObjectStore;

  public:

  /**
   * Construct a new data object.
   *
   * @param instanceId The ID of the instance.
   * @param id The ID of the data object.
   * @param localSlot The local memory slot of the data object.
   */
  DataObject(L0::Instance::instanceId_t instanceId, blockId id, std::shared_ptr<L0::LocalMemorySlot> localSlot)
    : _instanceId(instanceId),
      _id(id),
      _size(0),
      _localSlot(localSlot),
      _globalSlot(nullptr)
  {
    if (_localSlot) _size = _localSlot->getSize();
  }

  /**
   * Get the owner instance ID of the data object.
   *
   * @returns The owner instance ID of the data object.
   */
  __INLINE__ L0::Instance::instanceId_t getInstanceId() const { return _instanceId; }

  /**
   * Get the ID of the data object.
   *
   * @returns The ID of the data object.
   */
  __INLINE__ blockId getId() const { return _id; }

  /**
   * Get the local memory slot of the data object.
   *
   * @returns The local memory slot of the data object.
   */
  __INLINE__ L0::LocalMemorySlot &getLocalSlot() const { return *_localSlot; }

  private:

  /**
   * The ID of the instance owning the data object.
   */
  const L0::Instance::instanceId_t _instanceId;

  /**
   * The ID of the data object.
   */
  const blockId _id;

  /**
   * The size of the data object.
   */
  size_t _size;

  /**
   * The local memory slot of the data object.
   */
  std::shared_ptr<L0::LocalMemorySlot> _localSlot;

  /**
   * The global memory slot of the data object.
   */
  std::shared_ptr<L0::GlobalMemorySlot> _globalSlot;
};

} // namespace HiCR::objectStore
