/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sharedMemory.hpp
 * @brief This is a minimal backend for shared memory multi-core support based on HWLoc and Pthreads
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once
#define ELEMENT_TYPE unsigned int

// this values are simple wild guesses
#define DEFAULT_MEMSLOTS 10+_size
#define DEFAULT_MSGSLOTS 10*_size

#include <cstring>
#include <errno.h>
#include <future>
#include <memory>
#include <iostream>

#include <lpf/core.h>
#include <lpf/collectives.h>

#include "hwloc.h"

#include <hicr/backend.hpp>
//#include <hicr/common/logger.hpp>

#define EXPECT_EQ( format, expected, actual ) \
    do {  \
        if ( (expected) != (actual) ) { \
        fprintf(stderr, "TEST FAILURE in " __FILE__ ":%d\n" \
                  "   Expected (%s) which evaluates to " format ", but\n" \
                  "   actual (%s) which evaluates to " format ".\n",  __LINE__, \
                  #expected, (expected), #actual, (actual) ); \
        abort(); } \
       } while(0)

namespace HiCR
{

namespace backend
{

namespace lpf
{


  struct LpfMemSlot
  {
    // LPF slot handle
    lpf_memslot_t lpfSlot;
    size_t size;
    // pointer to the buffer 
    //void * pointer; 
    // only needed if this slot gets
    // promoted to global for memcpy
    size_t targetRank;
  };

class LpfBackend final : public Backend
{

  private:
  std::map<memorySlotId_t, LpfMemSlot> _lpfLocalSlots;
  //memorySlotId_t _currentTagId = 0;
  const size_t _size;
  const size_t _rank;
  const lpf_t _lpf;

  // global receive message count
  size_t msgCount;

  /**
   * Map of global slot id and MPI windows
   */

  public:

  bool isMemorySlotValidImpl(const memorySlotId_t memorySlotId) const override {
      return true;
  }

  size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const {
    HICR_THROW_RUNTIME("This backend provides no support for memory spaces");
  }

  /**
   * Thread-safe map that stores all allocated or created local memory slots associated to this backend
   */
  //parallelHashMap_t<memorySlotId_t, globalLPFSlot_t> _localSlotMap;
  /**
   * Thread-safe map that stores all global memory slots promoted from previous local slots
   */
  std::map<memorySlotId_t, LpfMemSlot> _globalSlotMap;
  //std::map<memorySlotId_t, lpf_memslot_t> hicr2LpfSlotMap;

  /**
   * hicrSlotID2MsgCnt is a HiCR representation of the map
   * slotID -> receive message count.
   * It is used to reset the monotonically increasing
   * map in LPF, when necessary.
   */
  std::map<memorySlotId_t, size_t> hicrSlotId2MsgCnt;

  /**
   * The decision to resize memory register in the constructor
   * is because this call requires lpf_sync to become effective.
   * This makes it almost impossible to do local memory registrations with LPF.
   * On the other hand, the resize message queue could also be locally
   * made, and placed elsewhere.
   */
 LpfBackend(size_t size, size_t rank, lpf_t lpf) : _size(size), _rank(rank), _lpf(lpf) {

    lpf_err_t rc;
    const size_t msgslots = DEFAULT_MSGSLOTS;
    const size_t memslots = DEFAULT_MEMSLOTS;
    // at the moment, the number to register is a wild guess
    rc = lpf_resize_message_queue( _lpf, msgslots);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    // at the moment, the number to register is a wild guess
    rc = lpf_resize_memory_register( _lpf, memslots);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

    rc = lpf_sync( _lpf, LPF_SYNC_DEFAULT );
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

 }

 LpfBackend(size_t size, size_t rank, lpf_t lpf, size_t msgslots, size_t memslots) : _size(size), _rank(rank), _lpf(lpf) {

    lpf_err_t rc;
    // at the moment, the number to register is a wild guess
    rc = lpf_resize_message_queue( _lpf, msgslots);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    // at the moment, the number to register is a wild guess
    rc = lpf_resize_memory_register( _lpf, memslots);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

    rc = lpf_sync( _lpf, LPF_SYNC_DEFAULT );
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

 }

 ~LpfBackend() {
    // for (auto i : _lpfLocalSlots) {
    //     lpf_deregister(_lpf, i.second.lpfSlot);
    // }
    // for (auto i : _globalSlotMap) {
    //     lpf_deregister(_lpf, i.second.lpfSlot);
    // }
 }

  std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const {
      HICR_THROW_RUNTIME("This backend provides no support for processing units");
  }

  void queryResources() {}

  size_t getProcessId() {
    return _rank;
  }

  void exchangeGlobalMemorySlots(const tag_t tag) {

      // Obtaining local slots to exchange
      size_t localSlotCount = (int)_pendingLocalToGlobalPromotions[tag].size();

      // Obtaining the local slots to exchange per process in the communicator
      //std::vector<int> perProcessSlotCount(_size);
      /**
       * The following block is basically a verbose allgather from tests/functional/c99
       * in LPF repo
       * @ToDo: consider a more compact LPF allgather (or others)
       * for the future
       */

      // start allgather block
      //std::vector<memorySlotId_t> globalSlotIds;
      lpf_coll_t coll;
      lpf_err_t rc;
      std::vector<size_t> globalSlotCounts(_size);
      for (size_t i=0; i<_size; i++) globalSlotCounts[i] = 0;
      lpf_memslot_t src_slot = LPF_INVALID_MEMSLOT;
      lpf_memslot_t dst_slot = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_local_sizes = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_local_keys = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_global_sizes = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_global_keys = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_local_process_ids = LPF_INVALID_MEMSLOT;
      lpf_memslot_t slot_global_process_ids = LPF_INVALID_MEMSLOT;

      rc = lpf_register_global( _lpf, &localSlotCount, sizeof(size_t), &src_slot);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_register_global( _lpf, globalSlotCounts.data(), sizeof(size_t) * _size, &dst_slot );
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_collectives_init( _lpf, _rank, _size, 1, 0, sizeof(size_t) * _size, &coll );
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_allgather( coll, src_slot, dst_slot, sizeof(size_t), false );
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );

      lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      //rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_collectives_destroy(coll);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_deregister(_lpf, src_slot);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_deregister(_lpf, dst_slot);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      // end allgather block


      size_t globalSlotCount = 0;
      for (size_t i=0; i < _size; i++) { 
          globalSlotCount += globalSlotCounts[i];
      }
      std::vector<size_t> globalSlotCountsInBytes(globalSlotCount); // only used for lpf_allgatherv
      for (size_t i=0; i<globalSlotCount; i++) {
          globalSlotCountsInBytes[i] = globalSlotCounts[i] * sizeof(size_t);
      }


      // globalSlotSizes will hold exactly the union of all slot sizes at 
      // each process (zero or more) to become global.
      // That is, allgatherv is implemented here

      //start allgatherv for GlobalSizes
      std::vector<size_t> localSlotSizes(localSlotCount);
      std::vector<size_t> localSlotKeys(localSlotCount);
      std::vector<size_t> localSlotProcessId(localSlotCount);
      std::vector<size_t> globalSlotSizes(globalSlotCount);
      std::vector<size_t> globalSlotKeys(globalSlotCount);
      std::vector<void *> globalSlotPointers(globalSlotCount);
      std::vector<size_t> globalSlotProcessId(globalSlotCount);

      for (size_t i = 0; i < localSlotCount; i++) {
          auto key = _pendingLocalToGlobalPromotions[tag][i].first;
          localSlotKeys[i] = key;
          localSlotProcessId[i] = _rank;
          auto memorySlotId = _pendingLocalToGlobalPromotions[tag][i].second;
          auto promotedSlot =  _memorySlotMap.at(memorySlotId);
          localSlotSizes[i] = promotedSlot.size;
      }

      rc = lpf_register_local( _lpf, localSlotSizes.data(), localSlotCount * sizeof(size_t), &slot_local_sizes);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_register_global( _lpf, globalSlotSizes.data(), globalSlotCount * sizeof(size_t), &slot_global_sizes);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_collectives_init( _lpf, _rank, _size, 2/* will call gatherv 2 times */, 0, sizeof(size_t) * globalSlotCount, &coll );
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_allgatherv( coll, slot_local_sizes, slot_global_sizes, globalSlotCountsInBytes.data(), false/*exclude myself*/);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );


      rc = lpf_register_local( _lpf, localSlotProcessId.data(), localSlotCount * sizeof(size_t), &slot_local_process_ids);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_register_global( _lpf, globalSlotProcessId.data(), globalSlotCount * sizeof(size_t), &slot_global_process_ids);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_allgatherv( coll, slot_local_process_ids, slot_global_process_ids, globalSlotCountsInBytes.data(), false/*exclude myself*/);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );

      rc = lpf_collectives_destroy(coll);
      lpf_deregister(_lpf, slot_local_sizes);
      lpf_deregister(_lpf, slot_global_sizes);
      lpf_deregister(_lpf, slot_local_process_ids);
      lpf_deregister(_lpf, slot_global_process_ids);


      rc = lpf_register_local(_lpf, localSlotKeys.data(), localSlotCount * sizeof(size_t), &slot_local_keys);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_register_global( _lpf, globalSlotKeys.data(), globalSlotCount * sizeof(size_t), &slot_global_keys);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );

      rc = lpf_collectives_init( _lpf, _rank, _size, 1, 0, sizeof(size_t) * globalSlotCount, &coll );
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_allgatherv( coll, slot_local_keys, slot_global_keys, globalSlotCountsInBytes.data(), false/*exclude myself*/);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_collectives_destroy(coll);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );
      rc = lpf_deregister(_lpf, slot_local_keys);
      rc = lpf_deregister(_lpf, slot_global_keys);
      EXPECT_EQ( "%d", LPF_SUCCESS, rc );


      size_t localPointerPos = 0;
      for (size_t i = 0; i < globalSlotCount; i++)
      {
          // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.

          // Memory for this slot not yet allocated or registered
          if (globalSlotProcessId[i] != _rank) {
              globalSlotPointers[i] = new char[globalSlotSizes[i]]; //NULL;
              // optionally initialize here?
          }
          // Memory already allocated and locally registered
          else
          {
              const auto memorySlotId = _pendingLocalToGlobalPromotions[tag][localPointerPos++].second;
              // deregister locally as it will be registered globally
              lpf_deregister(_lpf, _lpfLocalSlots[memorySlotId].lpfSlot);
              _lpfLocalSlots.erase(memorySlotId);
              globalSlotPointers[i] = _memorySlotMap.at(memorySlotId).pointer;
              // optionally initialize here?
          }

          void * newBuffer = globalSlotPointers[i];

          size_t newSize = globalSlotSizes[i];

          auto globalSlotId = registerGlobalMemorySlot(
                  tag,
                  globalSlotKeys[i],
                  globalSlotPointers[i],
                  globalSlotSizes[i]);

          lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
          rc = lpf_register_global(_lpf, newBuffer, newSize, &newSlot);
          lpf_sync(_lpf, LPF_SYNC_DEFAULT);
          EXPECT_EQ( "%d", LPF_SUCCESS, rc );
          //auto tag = _currentTagId++;
          /**
           * It is essential to record who this memory slot
           * belongs to (process i). Without this record, future data
           * movement calls will not work. This is recorded in the
           * targetRank of the destination slot.
           */
          size_t remote = globalSlotProcessId[i];
          // Registering global slot

          _globalSlotMap[globalSlotId] = LpfMemSlot {.lpfSlot = newSlot, .size = newSize, /*.pointer = newBuffer, */ .targetRank = remote};
      } 

      size_t msg_cnt;
      for (auto i : _globalSlotMap) {
          lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, i.second.lpfSlot);
          hicrSlotId2MsgCnt[i.first] = msg_cnt;
      }
  }

  void memcpyImpl(
    memorySlotId_t destination,
    const size_t dst_offset,
    const memorySlotId_t source,
    const size_t src_offset,
    const size_t size) {

    /**
     * For a put:
     * 1. The destination slot needs to be global
     * 2. The source slot may be either local or global
     * For a get:
     * 1. The destination slot may be either local or global
     * 2. The source slot needs to be global
     *
     * -> These principles determine the search in the hash maps
     *  and the error handling
     */

    bool isSourceGlobalSlot = _globalSlotMap.contains(source);
    bool isDestinationGlobalSlot = _globalSlotMap.contains(destination);

    // Checking whether source and/or remote are remote
    bool isSourceRemote = isSourceGlobalSlot ? _globalSlotMap[source].targetRank != _rank : false;
    bool isDestinationRemote = isDestinationGlobalSlot ? _globalSlotMap[destination].targetRank != _rank : false;

    // Sanity checks
    if (isSourceRemote && isDestinationGlobalSlot == false) HICR_THROW_LOGIC("Trying to use MPI backend in remote operation to with a destination slot(%lu) that has not been registered as global.", destination);
    if (isSourceRemote == true && isDestinationRemote == true) HICR_THROW_LOGIC("Trying to use MPI backend perform a remote to remote copy between slots (%lu -> %lu)", source, destination);

    // Calculating pointers
    LpfMemSlot dstSlot = (isDestinationGlobalSlot? _globalSlotMap.at(destination) : _lpfLocalSlots.at(destination));
    LpfMemSlot srcSlot = (isSourceGlobalSlot? _globalSlotMap.at(source) : _lpfLocalSlots.at(source));


    // Perform a get if the source is remote and destination is local
    if (isSourceGlobalSlot == true && isDestinationRemote == false)
    {
        lpf_get( _lpf, srcSlot.lpfSlot, src_offset, dstSlot.targetRank, dstSlot.lpfSlot, dst_offset, size, LPF_MSG_DEFAULT);
    }
    // Perform a put if source is local and destination is global
    else if (isSourceRemote == false && isDestinationGlobalSlot == true)
    {
        lpf_put( _lpf, srcSlot.lpfSlot, src_offset, dstSlot.targetRank, dstSlot.lpfSlot, dst_offset, size, LPF_MSG_DEFAULT);
    }
    else {
        std::cerr << "Did not find a suitable memcpy operation\n";
        abort();
    }

  }

  /**
   * ToDo: Implement tags in LPF !!!
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
      lpf_sync(_lpf, LPF_SYNC_DEFAULT);
  }

  /*
  void fenceImpl(const tag_t tag, size_t expected_msgs) {

      size_t latest_msg_cnt;
      lpf_get_rcvd_msg_count(_lpf, &latest_msg_cnt);

      while (expected_msgs > (latest_msg_cnt-msgCount)) {
          lpf_get_rcvd_msg_count(_lpf, &latest_msg_cnt);
      }
      msgCount = latest_msg_cnt;

  }
  */

  __USED__ inline void registerLocalMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) override
  {
      createMemorySlot(addr, size, memSlotId);
  }

  __USED__ inline void queryMemorySlotUpdatesImpl(const memorySlotId_t memorySlotId) override
  {
  }

  __USED__ inline void deregisterLocalMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void freeLocalMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory freeing");
  }

  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size, const memorySlotId_t memSlotId) override
  {
      HICR_THROW_RUNTIME("This backend provides no support for memory allocation");
  }

    __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
    {
        // No memory spaces are provided by this backend
        return memorySpaceList_t({});
    }
    memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size)  {
        
        memorySlotId_t m = 0; return m;}

    __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
    {
        // No compute resources are offered by the LPF backend
        return computeResourceList_t({});
    }

    void createMemorySlot(void *const addr, const size_t size, memorySlotId_t memorySlotId)  {
        lpf_memslot_t lpfSlot = LPF_INVALID_MEMSLOT;
        auto rc = lpf_register_local( _lpf, addr, size, &lpfSlot );
        if (rc != LPF_SUCCESS) {
          std::cerr << "lpf_register_local failed\n";
          std::abort();
        }
        _lpfLocalSlots[memorySlotId] = LpfMemSlot {.lpfSlot = lpfSlot, .size = size/*, .pointer = addr*/};

    }

    void *getMemorySlotLocalPointer(const memorySlotId_t memorySlotId) const {return nullptr;}

    void syncReceivedMessages(memorySlotId_t memorySlotId) override {
        size_t recvMsgCount = getRecvMsgCount(memorySlotId);
        _memorySlotMap.at(memorySlotId).messagesRecv  = recvMsgCount;
    }

    size_t getRecvMsgCount(const memorySlotId_t memorySlotId) override {
        size_t msg_cnt;
        lpf_memslot_t lpfSlotId = _globalSlotMap.at(memorySlotId).lpfSlot; //hicr2LpfSlotMap[memorySlotId];
        lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlotId);
        return msg_cnt - hicrSlotId2MsgCnt[lpfSlotId];
    }

    void flush() override {
        lpf_flush(_lpf);
    }

};

} // namespace lpf
} // namespace backend
} // namespace HiCR
