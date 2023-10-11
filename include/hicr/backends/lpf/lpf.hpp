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
    void * pointer; 
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
  std::map<memorySlotId_t, lpf_memslot_t> hicr2LpfSlotMap;

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
//     for (auto i : _localSlotMap) {
//         lpf_deregister(_lpf, i.second.lpfSlot);
     //}
   //  for (auto i : _globalSlotMap) {
   //      lpf_deregister(_lpf, i.second.lpfSlot);
   //  }
 }

  std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const {
      HICR_THROW_RUNTIME("This backend provides no support for processing units");
  }

  void queryResources() {}

  size_t getProcessId() {
    return _rank;
  }

  void exchangeGlobalMemorySlots(const tag_t tag) {

    std::cout << "Rank " << _rank << " entering exchange\n";

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
      std::cout << "Rank " << _rank << "globalSlotCount[" << i << "] = " << globalSlotCounts[i] << std::endl;
    }
    std::cout << "Global slot count = " << globalSlotCount << std::endl;
    std::vector<size_t> globalSlotCountsInBytes(globalSlotCount); // only used for lpf_allgatherv
    for (size_t i=0; i<globalSlotCount; i++) globalSlotCountsInBytes[i] = globalSlotCounts[i] * sizeof(size_t);

    // globalSlotSizes will hold exactly the union of all slot sizes at 
    // each process (zero or more) to become global.
    // That is, allgatherv is implemented here

    //start allgatherv for GlobalSizes
    std::vector<size_t> localSlotSizes(localSlotCount);
    std::vector<size_t> localSlotKeys(localSlotCount);
    std::vector<size_t> globalSlotSizes(globalSlotCount);
    std::vector<size_t> globalSlotKeys(globalSlotCount);
    for (size_t i = 0; i < localSlotCount; i++) {
        auto key = _pendingLocalToGlobalPromotions[tag][i].first;
        localSlotKeys[i] = key;
        auto memorySlotId = _pendingLocalToGlobalPromotions[tag][i].second;
        auto promotedSlot =  _memorySlotMap.at(memorySlotId);
        localSlotSizes[i] = promotedSlot.size;
        std::cout << "Rank " << _rank << " local slot size[" << i << "] = " << localSlotSizes[i] << std::endl;
    }
    rc = lpf_register_local( _lpf, localSlotSizes.data(), localSlotCount * sizeof(size_t), &slot_local_sizes);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_register_global( _lpf, globalSlotSizes.data(), globalSlotCount * sizeof(size_t), &slot_global_sizes);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    std::cout << "Collectives init max = " << sizeof(size_t) * globalSlotCount << std::endl;
    rc = lpf_collectives_init( _lpf, _rank, _size, 1/* will call gatherv 2 times */, 0, sizeof(size_t) * globalSlotCount, &coll );
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_allgatherv( coll, slot_local_sizes, slot_global_sizes, globalSlotCountsInBytes.data(), false/*exclude myself*/);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

    for (size_t i=0; i<globalSlotCount; i++) {
        std::cout << "GlobalSlotSizes[" << i << "] = " << globalSlotSizes[i] << std::endl;
    }


    rc = lpf_collectives_destroy(coll);
    lpf_deregister(_lpf, slot_local_sizes);
    lpf_deregister(_lpf, slot_global_sizes);


    rc = lpf_register_local(_lpf, localSlotKeys.data(), localSlotCount * sizeof(size_t), &slot_local_keys);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_register_global( _lpf, globalSlotKeys.data(), globalSlotCount * sizeof(size_t), &slot_global_keys);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );

    rc = lpf_collectives_init( _lpf, _rank, _size, 1/* will call gatherv 2 times */, 0, sizeof(size_t) * globalSlotCount, &coll );
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_allgatherv( coll, slot_local_keys, slot_global_keys, globalSlotCountsInBytes.data(), false/*exclude myself*/);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_sync(_lpf, LPF_SYNC_DEFAULT);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_collectives_destroy(coll);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );



    std::vector<void *> globalSlotPointers(globalSlotCount);
    std::vector<size_t> globalSlotProcessId(globalSlotCount);


    // it should be okay do deduce from the allgat way the slot process IDs
    // (Sergio's implementation uses another allgatherv for this)
    for (size_t i=0, currentRank=0, ind=0; i<globalSlotCount ; i++, currentRank++) {
        for (size_t j=0; j<globalSlotCounts[j]; j++, ind++) {
            globalSlotProcessId[ind] = currentRank;
        }
    }

    size_t localPointerPos = 0;
    for (size_t i = 0; i < globalSlotPointers.size(); i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.
      if (globalSlotProcessId[i] != _rank)
        globalSlotPointers[i] = NULL;
      else
      {
        const auto memorySlotId = _pendingLocalToGlobalPromotions[tag][localPointerPos++].second;
        globalSlotPointers[i] = _memorySlotMap.at(memorySlotId).pointer;
      }
    }

    for (size_t i = 0; i < localSlotCount; i++) {
        if (_pendingLocalToGlobalPromotions.find(i) == _pendingLocalToGlobalPromotions.end()) {
            std::cerr << "Error looking for entry in pendingLocalToGlobalPromotions!\n";
            std::abort();
        }
        auto memorySlotId = _pendingLocalToGlobalPromotions[tag][i].second;
        std::cout << "aLocalSlot = " << memorySlotId << " from local slot i = " << i <<  std::endl;
        if (_memorySlotMap.find(memorySlotId) == _memorySlotMap.end()) {
            std::cerr << "Error looking for locally registered slot!\n";
            std::abort();
        }
        auto promotedSlot =  _memorySlotMap[memorySlotId];
        localSlotSizes[i] = promotedSlot.size;
        std::cout << "Rank " << _rank << " Setting slot size = " << i << " = " << localSlotSizes[i] << std::endl;
    }

    for (size_t i=0; i<localSlotCount; i++) {
        std::cout << "RANK " << _rank << " ->  localSlotSizes[" << i << "] = " << localSlotSizes[i] << std::endl;
    }



    //for (size_t i=0, index=0; i<_size; i++) {
        // Make local slots of rank i global !!!
    //for (size_t i=0, gsIndex = 0; i<_size; i++) {
    for (size_t i=0, ind=0; i<globalSlotCount; i++) {

        std::cout << "Register global slot " << i << " with size = " << globalSlotSizes[i] << std::endl;
      auto globalSlotId = registerGlobalMemorySlot(
        tag,
        globalSlotKeys[i],
        globalSlotPointers[i],
        globalSlotSizes[i]);

      std::cout << "Rank " << _rank << " globalSlotId = " << globalSlotId << std::endl;
        lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
        memorySlotId_t memorySlotId;
        void * newBuffer = nullptr;
        size_t newSize = globalSlotSizes[i];
        std::cout << "newSize = " << globalSlotSizes[i] << " for i = " << i << std::endl;
        /**
         * only allocate local buffers where needed
         * that is: where the rank is promoting its local
         * slot to a global slot
         */
        if (globalSlotProcessId[i] == _rank) {
            if (_pendingLocalToGlobalPromotions[tag].size() <  ind + 1) {
                std::cerr << "Running out of bounds of pending map with pending size for this tag = " << _pendingLocalToGlobalPromotions[tag].size() << std::endl;
                std::abort();
            }
            memorySlotId = _pendingLocalToGlobalPromotions[tag][ind].second;
            if (_lpfLocalSlots.find(memorySlotId) == _lpfLocalSlots.end()) {
                std::cout << "aLocalSlot: " << memorySlotId << std::endl;
                std::cout << "FIRST: " << _pendingLocalToGlobalPromotions[tag][ind].first << std::endl;
                std::cout << "SECOND: " <<_pendingLocalToGlobalPromotions[tag][ind].second << std::endl;
                std::cerr << "Could not find a locally registered slot, exiting\n";
                std::abort();
            }
            auto promotedSlot = _lpfLocalSlots[memorySlotId];
            newBuffer = promotedSlot.pointer;
            // local memslot can be locally deregistered
            std::cout << "Rank " << _rank << " Try to deregister slot with key " << memorySlotId << std::endl;
            rc = lpf_deregister(_lpf, _lpfLocalSlots[memorySlotId].lpfSlot);
            EXPECT_EQ( "%d", LPF_SUCCESS, rc );
            // remove local slot from local slot hash table
            _lpfLocalSlots.erase(memorySlotId);
            ind++;
        }
        else {
            newBuffer = malloc(newSize);
        }

        std::cout << "Rank " << _rank << " Will register globally a slot of size " << newSize << std::endl;
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

        _globalSlotMap[globalSlotId] = LpfMemSlot {.lpfSlot = newSlot, .size = newSize, .pointer = newBuffer, .targetRank = remote};
        //globalSlotIds.push_back(tag);
        hicr2LpfSlotMap[globalSlotId] = newSlot;
    } 

    size_t msg_cnt;
    for (auto i : _globalSlotMap) {
        lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, i.second.lpfSlot);
        hicrSlotId2MsgCnt[i.first] = msg_cnt;
    }

    std::cout << "Rank " << _rank << " leaving exchange\n";
    }

  void memcpyImpl(
    memorySlotId_t destination,
    const size_t dst_offset,
    const memorySlotId_t source,
    const size_t src_offset,
    const size_t size) {

    LpfMemSlot dstSlot;
    /**
     * 1. The destination slot needs to be global
     * 2. The source slot may be either local or global
     * -> These principles determine the search in the hash maps
     *  and the error handling
     */
    std::cout << "Rank " << _rank << " in memcpyImpl with dest slot id = " << destination << std::endl;
    if ( _globalSlotMap.find(destination) == _globalSlotMap.end()) {
      std::cerr << "Destination slot: Cannot find entry in global slot map\n";
      std::abort();
    }
    else {
      dstSlot =  _globalSlotMap[destination];
    }

    LpfMemSlot srcSlot;
    if (_globalSlotMap.find(source) == _globalSlotMap.end()) {
      if (_lpfLocalSlots.find(source) == _lpfLocalSlots.end()) {
        std::cerr << "Source slot: Cannot find entry in local or global slot Map\n";
        std::abort();
      }
      else {
        srcSlot = _lpfLocalSlots[source];
      }
    }
    else {
     srcSlot = _globalSlotMap[source];
    }
    if (srcSlot.targetRank == SIZE_MAX)  {
        std::cerr << "target locality for lpf_put illegal, abort!\n";
        std::abort();
    }
    lpf_put( _lpf, srcSlot.lpfSlot, src_offset, dstSlot.targetRank, dstSlot.lpfSlot, dst_offset, size, LPF_MSG_DEFAULT);

  }

  /**
   * ToDo: Implement tags in LPF !!!
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
      std::cout << "ENTER FENCEIMPL\n";

      lpf_sync(_lpf, LPF_SYNC_DEFAULT);

  }

  void fenceImpl(const tag_t tag, size_t expected_msgs) {

      size_t latest_msg_cnt;
      lpf_get_rcvd_msg_count(_lpf, &latest_msg_cnt);

      while (expected_msgs > (latest_msg_cnt-msgCount)) {
          lpf_get_rcvd_msg_count(_lpf, &latest_msg_cnt);
      }
      msgCount = latest_msg_cnt;

  }

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
        
        std::cout << "Call allocateMemorySlot\n";
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
        _lpfLocalSlots[memorySlotId] = LpfMemSlot {.lpfSlot = lpfSlot, .size = size, .pointer = addr};

    }

    void *getMemorySlotLocalPointer(const memorySlotId_t memorySlotId) const {return nullptr;}

    size_t getRecvMsgCount(const memorySlotId_t memorySlotId) override {
        size_t msg_cnt;
        lpf_memslot_t lpfSlotId = hicr2LpfSlotMap[memorySlotId];
        lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlotId);
        return msg_cnt - hicrSlotId2MsgCnt[lpfSlotId];
    }

};

} // namespace lpf
} // namespace backend
} // namespace HiCR
