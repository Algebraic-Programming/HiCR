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
 * @file objectStore.hpp
 * @brief Provides functionality for a block object store over HiCR
 * @author A. N. Yzelman, O. Korakitis
 * @date 29/11/2024
 */

#pragma once

#include <atomic>

#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/core/instance.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/frontends/objectStore/dataObject.hpp>

enum
{
  /**
   * The number of bits used to store the instance ID in the compound ID.
   */
  OBJECT_STORE_KEY_INSTANCE_ID_BITS = 32
};

namespace HiCR::objectStore
{

/**
 * The type of a compound ID, used to uniquely identify a block in the object store.
 * The 64bit compound ID is formed by combining the instance ID (32bits) and the block ID (the other 32 bits).
 */
using compoundId_t = uint64_t;

/**
 * This is the front-end, managing entity for the object store.
 * Multiple instances of this class can be created, one for each tag.
 *
 * It has a role similar to core HiCR's L1 Manager classes, hence it
 * uses these managers as dependencies. The ObjectStore is responsible
 * for the management of data-objects/blocks.
 *
 * A block is a globally unique reference to a memory region that exists
 * somewhere on the system. It is created via a call to #createObject, and
 * published (i.e. made available for access) via a call to #publish.
 *
 * A block has an owner, which initially is the worker that created the block
 * via a call to #publish. An owner of a block is not necessarily an active
 * participant on all activities on said block-- in fact, most operations on
 * blocks are asynchronous while the core functionality, i.e., reading a block
 * and benefit of one-sided fencing. Staying passive while subject to remote
 * reads, however, does require the underlying system to natively support it.
 *
 * A block at a worker which is not the owner of the block will, after the
 * first call to #get, return a pointer to the block contents for read-only
 * access. At non-owner locations, the returned memory pointer need not point
 * to a copy of the owner's memory -- consider, e.g., a shared-memory object
 * store impementation, or a sophisticated network fabric with GAS-like
 * functionalities.
 *
 * The memory area returned by #get will not necessarily be kept up to date
 * with the contents at the owner side. Successive calls to #get may be used to
 * re-synchronise with the owner.
 *
 * A call to #publish is not collective, meaning that remote workers will not
 * immediately be able to refer to any remotely published block. For remote
 * workers to be able to refer to a block, a serialized handle to the block
 * should be copied to the remote workers that want to make use of it
 * (e.g., by using channels or raw memory copies).
 *
 * Therefore, such handles of the #HiCR::objectStore::DataObject shall be copyable, in the exact sense
 * of the corresponding C++ type trait. This informally means it can both be the
 * target for raw memcpy calls as well as the target for inter-worker
 * communication. Copying allows for remote workers to refer to blocks it did
 * not (initially) own.
 *
 * Multiple instances of the same block on the same worker will always refer to
 * the same memory (i.e., the pointer returned by #get will equal when called on
 * the same blocks on the same worker).
 *
 * On non-owner workers, a call to #get \em asynchronously initiates the data
 * movement from the owner to the non-owner, so that the non-owner local block
 * memory reflects the contents of the owner of the block. This copy is only
 * guaranteed to have completed at the end of a subsequent call to #fence.
 * Before this call to #fence, the contents of the pointer returned by #get are
 * undefined.
 *
 * Accordingly, the block data at the owner worker should be immutable while
 * other non-owner workers have issued calls to #get-- otherwise, partially
 * updated and partially non-updated contents may be received, leading to
 * inconsistent states. To indicate to the owner that any pending reads have
 * completed (and thus that it may safely modify its data), again a call to
 * #fence should be made.
 *
 * A call to #fence is a blocking function. A non-blocking variant may in the
 * future be provided by test_fence().
 */
class ObjectStore
{
  public:

  /**
   * Constructor
   *
   * @param[in] communicationManager The communication manager of the HiCR instance.
   * @param[in] tag The tag to associate with the object store.
   * @param[in] memoryManager The memory manager of the HiCR instance.
   * @param[in] memorySpace The memory space the object store operates in.
   * @param[in] instanceId The ID of the instance running the object store. (Created objects will have this instance ID)
   */
  ObjectStore(CommunicationManager        &communicationManager,
              GlobalMemorySlot::tag_t      tag,
              MemoryManager               &memoryManager,
              std::shared_ptr<MemorySpace> memorySpace,
              Instance::instanceId_t       instanceId)
    : _memoryManager(memoryManager),
      _communicationManager(communicationManager),
      _tag(tag),
      _memorySpace(memorySpace),
      _instanceId(instanceId)
  {}

  /**
   * Default Destructor
   */
  ~ObjectStore() = default;

  /**
   * Get the memory space the object store operates in.
   *
   * @returns The memory space the object store operates in.
   */
  [[nodiscard]] std::shared_ptr<MemorySpace> getMemorySpace() const { return _memorySpace; }

  /**
   * Creates a new data object from a given memory allocation.
   * The calling worker will be the owner of the returned Data Object.
   *
   * @param[in] ptr The pointer to the memory region.
   * @param[in] size The size of the memory region.
   * @param[in] id The ID of the block to be created.
   *
   * @returns A DataObject referring to the memory region.
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<DataObject> createObject(void *ptr, size_t size, blockId id)
  {
    // Register the given allocation as a memory slot
    auto slot = _memoryManager.registerLocalMemorySlot(_memorySpace, ptr, size);

    return std::make_shared<DataObject>(_instanceId, id, slot);
  }

  /**
   * Creates a new data object from an existing LocalMemorySlot.
   * The calling worker will be the owner of the returned Data Object.
   *
   * @param[in] slot The local memory slot to create the block from.
   * @param[in] id The ID of the block to be created.
   *
   * @returns A DataObject referring to the memory slot.
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<DataObject> createObject(std::shared_ptr<LocalMemorySlot> slot, blockId id)
  {
    return std::make_shared<DataObject>(_instanceId, id, slot);
  }

  /**
   * Publishes a block to the object store.
   *
   * A call to this function is \em not thread-safe.
   *
   * @param[in] dataObject The data object to publish.
   */
  __INLINE__ void publish(std::shared_ptr<DataObject> dataObject)
  {
    blockId objectId = dataObject->_id;

    if (dataObject->_globalSlot)
    {
      _communicationManager.destroyPromotedGlobalMemorySlot(dataObject->_globalSlot);
      HICR_THROW_LOGIC("Trying to publish a block that has already been published."); // FIXME: What do we do with republishing?
    }
    // One-sided publish of the global slot
    auto globalSlot = _communicationManager.promoteLocalMemorySlot(dataObject->_localSlot, _tag);

    dataObject->_globalSlot = globalSlot;
    _globalObjects.insert({dataObject->_instanceId << OBJECT_STORE_KEY_INSTANCE_ID_BITS | objectId, dataObject});
  }

  /**
   * Retrieve a pointer to the block contents.
   *
   * Multiple instances of the same block on the same worker will always refer to
   * the same memory (i.e., the pointer returned by #get will equal when called on
   * the same blocks on the same worker). If the block is not found in the object store,
   * ie. it has not been published, or the relevant update has not finished, a nullptr
   * will be returned.
   *
   * On non-owner workers, a call to #get \em asynchronously initiates the data
   * movement from the owner to the non-owner, so that the non-owner local block
   * memory reflects the contents of the owner of the block. This copy is only
   * guaranteed to have completed at the end of a subsequent call to #fence.
   * Before this call to #fence, the contents of the pointer returned by #get are
   * undefined.
   *
   * @param[in] dataObject The data object to get.
   *
   * @returns A pointer to the memory slot that contains the block contents, or a nullptr if the block is not found.
   *
   * The contents are guaranteed to match with the owner's after the next call
   * to #fence over the given \a ID.
   *
   * A call to this function implies \f$ \mathcal{O}(n) \f$ runtime, where
   * \f$ n \f$ is the byte size of this block.
   *
   * \note The intent is that this upper bound on work occurs at most once
   *       per block, and only in the case that retrieval first requires the
   *       allocation of a local buffer.
   *
   * A call to this function is non-blocking, since the contents of the memory
   * the return value points to are undefined until the next call to #fence.
   * This function is one-sided and (thus) \em not collective.
   *
   * Repeated calls to this function with the same \a ID but without any
   * intermediate calls to #fence with that \a ID will behave as though only
   * a single call was made.
   *
   * A call to this function shall be thread-safe.
   *
   * \note The thread-safety plus re-entrant-safety means the implementation
   *       of this function must include an atomic guard that is unlocked at
   *       a call to #fence.
   */
  __INLINE__ std::shared_ptr<LocalMemorySlot> get(DataObject &dataObject)
  {
    // Deduce globally unique block ID for the GlobalMemorySlot key, by combining the given instance and block IDs
    compoundId_t compoundId = dataObject._instanceId << OBJECT_STORE_KEY_INSTANCE_ID_BITS | dataObject._id;

    _globalObjects[compoundId] = std::make_shared<DataObject>(dataObject);

    // If the data object has never been fetched before, allocate a local memory slot for it
    if (dataObject._localSlot == nullptr)
    {
      // Allocate a local memory slot for the block, associate it with the global slot and add it to the blocks of the given instance
      auto newSlot          = _memoryManager.allocateLocalMemorySlot(_memorySpace, dataObject._size);
      dataObject._localSlot = newSlot;
    }

    if (dataObject._globalSlot == nullptr) HICR_THROW_LOGIC("Trying to get a block that has not been properly transferred.");

    // Initiate the data movement from the owner to the current worker
    _communicationManager.memcpy(dataObject._localSlot, 0, dataObject._globalSlot, 0, dataObject._size);
    return dataObject._localSlot;

    // Notes: Consider a variant (maybe hidden under another layer/wrapper) that allows to get portion
    // of the block, using offset and size
  }

  /**
   * Destroy a data object. This is a local operation, does not affect other copies of the data object.
   * The data object is removed from the object store and the associated memory slots are freed.
   *
   * The local slot is only freed if the data object is a fetched object, originating from a call to #get.
   * On the owner instance, the user is responsible for freeing the local memory slot.
   * The global slot is always destroyed, if present.
   *
   * @param[in] dataObject The data object to destroy.
   */
  __INLINE__ void destroy(DataObject &dataObject)
  {
    // Only free the local memory slot if it is a fetched object, which means the local slot allocation
    // was done by the object store during a call to #get
    if (dataObject._instanceId != _instanceId)
    {
      if (dataObject._localSlot) _memoryManager.freeLocalMemorySlot(dataObject._localSlot);
    }

    if (dataObject._globalSlot) _communicationManager.destroyPromotedGlobalMemorySlot(dataObject._globalSlot);

    compoundId_t compoundId = dataObject._instanceId << OBJECT_STORE_KEY_INSTANCE_ID_BITS | dataObject._id;
    auto         it         = _globalObjects.find(compoundId);

    if (it != _globalObjects.end()) { _globalObjects.erase(it); }
  }

  /**
   * Produce a serialized representation (handle) of a data object.
   * This representation is trivially copyable and can be sent over the network.
   *
   * @param[in] dataObject The data object to serialize.
   * @returns A handle to the data object.
   */
  __INLINE__ handle serialize(DataObject &dataObject)
  {
    auto ret                      = handle{};
    ret.instanceId                = dataObject._instanceId;
    ret.id                        = dataObject._id;
    ret.size                      = dataObject._size;
    uint8_t *serializedGlobalSlot = _communicationManager.serializeGlobalMemorySlot(dataObject._globalSlot); // implicitly produces allocation
    std::memcpy(ret.serializedGlobalSlot, serializedGlobalSlot, sizeof(ret.serializedGlobalSlot));

    // free the allocation produced by serialize() TODO: dangerous, remove
    delete[] serializedGlobalSlot;
    return ret;
  }

  /**
   * Deserialize a handle to a data object.
   * The handle is a trivially copiable, serialized representation of a data object.
   *
   * @param[in] handle The handle to deserialize.
   * @returns A shared pointer to the deserialized data object.
   */
  __INLINE__ std::shared_ptr<DataObject> deserialize(const handle &handle)
  {
    auto dataObject   = std::make_shared<DataObject>(handle.instanceId, handle.id, nullptr);
    dataObject->_size = handle.size;
    dataObject->_globalSlot =
      _communicationManager.deserializeGlobalMemorySlot(reinterpret_cast<uint8_t *>(const_cast<uint8_t(*)[28 + sizeof(size_t)]>(&handle.serializedGlobalSlot)), _tag);
    return dataObject;
  }

  /**
   * Fences all block activity of the object store instance.
   *
   * This is a collective, blocking call. All workers that have communications
   * in the object store must make the same number of calls to #fence. This
   * (paradoxically) allows for fully asynchronous fencing.
   *
   * \warning I.e., a call to fence on one party may exit before a matching
   *          call elsewhere has finished: a call to this function does
   *          \em not imply a barrier.
   *
   * A call to this function is \em not thread-safe.
   *
   * @returns <tt>true</tt> if the fence has completed; <tt>false</tt> otherwise.
   */
  __INLINE__ bool fence()
  {
    _communicationManager.fence(_tag);
    return true;
  }

  /**
   * Fences locally on a specific data object.
   *
   * This is a one-sided, blocking call; returning from this function
   * indicates that, specific to this data object, incoming memory movement (ie. a get())
   * has completed.
   *
   * @param[in] dataObject The data object to fence.
   * @returns <tt>true</tt> if the fence has completed; <tt>false</tt> otherwise.
   */
  __INLINE__ bool fence(const std::shared_ptr<DataObject> &dataObject)
  {
    // Use the zero-cost variant of fence with (sent,recv) = (0,1)
    _communicationManager.fence(dataObject->_localSlot, 0, 1);
    return true;
  }

#ifdef HICR_ENABLE_NONBLOCKING_FENCE // Dummy flag for when we implement non-blocking fence
  /**
   * The non-blocking variant of #fence.
   *
   * @param[in] tag The tag to test for.
   *
   * @returns <tt>true</tt> if the fence has completed;
   * @returns <tt>false</tt> if the fence has not completed.
   *
   * A call to this function shall incur \f$ \mathcal{O}(1) \f$ work.
   *
   * This is a non-blocking and asynchronous call. If the call returns
   * <tt>false</tt>, the call shall have no side-effects (i.e., the state of
   * the unified layer is as though the call were never made).
   *
   * If the call returns <tt>true</tt>, the semantics and effects match that
   * of #update.
   *
   * A call to this function is \em not thread-safe.
   */
  static bool test_fence(const Tag &tag);
#endif

  private:

  /**
   * The associated memory manager.
   */
  MemoryManager &_memoryManager;

  /**
   * The associated communication manager.
   */
  CommunicationManager &_communicationManager;

  /**
   * The tag to associate with the objectStore instance.
   *
   * \note If we remove collectives, we can probably remove the tag.
   */
  const GlobalMemorySlot::tag_t _tag;

  /**
   * The associated memory space the object store operates in.
   */
  const std::shared_ptr<MemorySpace> _memorySpace;

  /**
   * Directory of blocks. TODO: Might remove, or keep as the responsibility of the object store to track the blocks.
   *
   * \note If we have guarantees about instance ids, we can use a vector instead of a map.
   *       Same for blockIds.
   */
  std::map<compoundId_t, std::shared_ptr<DataObject>> _globalObjects{};

  /**
   * The instance ID of the current instance owning the object store.
   */
  const Instance::instanceId_t _instanceId;
};

} // namespace HiCR::objectStore
