/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mpi.hpp
 * @brief This is a minimal backend for MPI support
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <cstring>
#include <stdio.h>
#include <mpi.h>

#include <hicr/backend.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

/**
 * Implementation of the HiCR MPI backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class MPI final : public Backend
{
  public:

  /**
   * Constructor for the mpi backend.
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  MPI(MPI_Comm comm) : Backend(), _comm(comm)
  {
   MPI_Comm_size(_comm, &_size);
   MPI_Comm_rank(_comm, &_rank);
  }

  ~MPI() = default;

  private:

  /**
   * Struct to hold relevant memory slot information
   */
  struct globalMPISlot_t
  {
   /**
    * Remembers the MPI of which this memory is local
    */
   int rank;

   /**
    * Stores the MPI window to use with this slot
    */
   MPI_Win* window;
  };

  /**
   * Default MPI communicator to use for this backend
   */
  const MPI_Comm _comm;

  /**
   * Number of MPI processes in the communicator
   */
  int _size;

  /**
   * MPI rank corresponding to this process
   */
  int _rank;

  /**
   * Map of global slot id and MPI windows
   */
  std::map<memorySlotId_t, globalMPISlot_t> _globalMemorySlotMPIWindowMap;

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory spaces");
  }

  /**
   * The MPI backend offers no memory spaces
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // No compute resources are offered by the MPI backend
    return computeResourceList_t({});
  }

  /**
   * The MPI backend offers no memory spaces
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // No memory spaces are provided by this backend
    return memorySpaceList_t({});
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    HICR_THROW_RUNTIME("This backend provides no support for processing units");
  }

  __USED__ inline void memcpyImpl(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size) override
  {
   // Sanity checks
   if (_globalMemorySlotMPIWindowMap.contains(source)      == false) HICR_THROW_LOGIC("Trying to use MPI backend to with a source slot(%lu) that has not been registered as global.", source);
   if (_globalMemorySlotMPIWindowMap.contains(destination) == false) HICR_THROW_LOGIC("Trying to use MPI backend to with a destination slot(%lu) that has not been registered as global.", destination);

   // Checking whether source and/or remote are remote
   bool isSourceRemote      = _globalMemorySlotMPIWindowMap[source].rank      != _rank;
   bool isDestinationRemote = _globalMemorySlotMPIWindowMap[destination].rank != _rank;

   // MPI backend should be used for local<->remote communications to determine whether Put or get should be used. This can be relaxed in the future, but for now we keep this in mind
   if (isSourceRemote == false && isDestinationRemote == false) HICR_THROW_LOGIC("Trying to use MPI backend perform a local to local copy between slots (%lu -> %lu)", source, destination);
   if (isSourceRemote == true  && isDestinationRemote == true)  HICR_THROW_LOGIC("Trying to use MPI backend perform a remote to remote copy between slots (%lu -> %lu)", source, destination);

   // Perform a get if the source is remote and destination is local
   if (isSourceRemote == true && isDestinationRemote == false)
   {
    // Calculating source pointer (with offset)
    auto destinationPointer = (void*) (((uint8_t*)_memorySlotMap.at(destination).pointer) + dst_offset);

    // Executing the get operation
    auto status = MPI_Get(
      destinationPointer,
      size,
      MPI_BYTE,
      _globalMemorySlotMPIWindowMap[source].rank,
      src_offset,
      size,
      MPI_BYTE,
      *_globalMemorySlotMPIWindowMap[destination].window);

    // Checking execution status
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Get (Slots %lu -> %lu)", source, destination);
   }

   // Perform a put if source is local and destination is remote
   if (isSourceRemote == false && isDestinationRemote == true)
   {
    // Calculating source pointer (with offset)
    auto sourcePointer = (void*) (((uint8_t*)_memorySlotMap.at(destination).pointer) + src_offset);

    // Executing the get operation
    auto status = MPI_Put(
      sourcePointer,
      size,
      MPI_BYTE,
      _globalMemorySlotMPIWindowMap[destination].rank,
      dst_offset,
      size,
      MPI_BYTE,
      *_globalMemorySlotMPIWindowMap[source].window);

    // Checking execution status
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Put (Slots %lu -> %lu)", source, destination);
   }
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlotId Identifier of the memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const memorySlotId_t memorySlotId) override
  {
  }

  /**
   * Implementation of the fence operation for the mpi backend. In this case, nothing needs to be done, as
   * the memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier of the new local memory slot
   */
  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size, const memorySlotId_t memSlotId) override
  {
     HICR_THROW_RUNTIME("This backend provides no support for memory allocation");
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier for the new local memory slot
   */
  __USED__ inline void registerLocalMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) override
  {
    // Nothing to do here for this backend
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
    // Nothing to do here for this backend
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * This is a collective function that will block until the user-specified expected slot count is found.
   *
   * \param[in] tag Identifies a particular subset of global memory slots, and returns it
   * \param[in] localMemorySlotIds Provides the local slots to be promoted to global and exchanged by this HiCR instance
   * \param[in] key The key to use for the provided memory slots. This key will be used to sort the global slots, so that the ordering is deterministic if all different keys are passed.
   * \returns A map of global memory slot arrays identified with the tag passed and mapped by key.
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const tag_t tag, const globalKey_t key, const std::vector<memorySlotId_t> localMemorySlotIds)
  {
   // Obtaining local slots to exchange
   int localSlotCount = (int)localMemorySlotIds.size();

   // Obtaining the local slots to exchange per process in the communicator
   std::vector<int> perProcessSlotCount(_size);
   MPI_Allgather(&localSlotCount, 1, MPI_INT, perProcessSlotCount.data(), 1, MPI_INT, _comm);

   // Calculating respective offsets
   std::vector<int> perProcessSlotOffsets(_size);
   int currentOffset = 0;
   for (int i = 0; i < _size; i++)
   {
    perProcessSlotOffsets[i] += currentOffset;
    currentOffset += perProcessSlotCount[i];
   }

   // Calculating number of global slots
   int globalSlotCount = 0;
   for (const auto count : perProcessSlotCount) globalSlotCount += count;

   // Allocating storage for local and global memory slot sizes
   std::vector<size_t> localSlotSizes(localSlotCount);
   std::vector<size_t> globalSlotSizes(globalSlotCount);

   // Filling in the local size storage
   for (size_t i = 0; i < localSlotSizes.size(); i++) localSlotSizes[i] = _memorySlotMap.at(localMemorySlotIds[i]).size;

   // Exchanging global sizes
   MPI_Allgatherv(localSlotSizes.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotSizes.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);

   // Allocating storage for local and global slot<->rank identification. Needed to know which owns the window
   std::vector<int> localSlotProcessId(localSlotCount);
   std::vector<int> globalSlotProcessId(globalSlotCount);

   // Filling in the local size storage
   for (size_t i = 0; i < localSlotProcessId.size(); i++) localSlotProcessId[i] = _rank;

   // Exchanging global slot process ids
   MPI_Allgatherv(localSlotProcessId.data(), localSlotCount, MPI_INT, globalSlotProcessId.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_INT, _comm);

   // Now also creating pointer vector for convenience
   std::vector<void*> globalSlotPointers(globalSlotCount);
   size_t localPointerPos = 0;
   for (size_t i = 0; i < globalSlotPointers.size(); i++) globalSlotPointers[i] = globalSlotProcessId[i] == _rank ? _memorySlotMap.at(localPointerPos++).pointer : NULL;

   // Debug info
   //for (size_t i = 0; i < globalSlotSizes.size(); i++) printf("Rank: %u, GlobalSlot %lu, Size: %lu, LocalPtr: 0x%lX, %s\n", _rank, i, globalSlotSizes[i], (uint64_t)globalSlotPointers[i], globalSlotProcessId[i] == _rank ? "x" : "");

   // Now creating global slots and their MPI windows
   for (size_t i = 0; i < globalSlotProcessId.size(); i++)
   {
    // Registering global slot
    auto globalSlotId = registerGlobalMemorySlot(
      tag,
      key,
      globalSlotPointers[i],
      globalSlotSizes[i]);

    // Creating new entry in the MPI Window map
    _globalMemorySlotMPIWindowMap[globalSlotId] = globalMPISlot_t { .rank = globalSlotProcessId[i], .window = new MPI_Win };

    // Creating MPI window
    int status = MPI_Win_create(
      globalSlotPointers[i],
      globalSlotProcessId[i] == _rank ? globalSlotSizes[i] : 0,
      1,
      MPI_INFO_NULL,
      _comm,
      _globalMemorySlotMPIWindowMap[globalSlotId].window);

    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI window on exchange global memory slots.");
   }

  }

  /**
   * Frees up a local memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
   HICR_THROW_RUNTIME("This backend provides no support for memory freeing");
  }

  /**
   * Checks whether the memory slot id exists and is valid.
   *
   * In this backend, this means that the memory slot was either allocated or created and it contains a non-NULL pointer.
   *
   * \param[in] memorySlotId Identifier of the slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const memorySlotId_t memorySlotId) const override
  {
    // Otherwise it is ok
    return true;
  }
};

} // namespace mpi
} // namespace backend
} // namespace HiCR
