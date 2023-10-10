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
#define DEFAULT_MEMSLOTS 3+_size
#define DEFAULT_MSGSLOTS 2*_size

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

/**
 * Internal representation of a memory slot for the shared memory backend
 */

struct LpfMemSlot : public HiCR::Backend::memorySlotStruct_t
{
    // LPF slot handle
    lpf_memslot_t lpfSlot;
    size_t size;
    // pointer to the buffer 
     void * buffer; 
    // only needed if this slot gets
    // promoted to global for memcpy
    size_t targetRank;
};

class LpfBackend final : public Backend
{

  private:
  memorySlotId_t _currentTagId = 0;
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
  parallelHashMap_t<memorySlotId_t, LpfMemSlot> _localSlotMap;
  /**
   * Thread-safe map that stores all global memory slots promoted from previous local slots
   */
  parallelHashMap_t<memorySlotId_t, LpfMemSlot> _globalSlotMap;
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
     for (auto i : _localSlotMap) {
         lpf_deregister(_lpf, i.second.lpfSlot);
     }
     for (auto i : _globalSlotMap) {
         lpf_deregister(_lpf, i.second.lpfSlot);
     }
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
    size_t *globalSlotCounts  = (size_t *) malloc(sizeof(size_t) * _size);
    for (size_t i=0; i<_size; i++) globalSlotCounts[i] = 0;
    lpf_memslot_t src_slot = LPF_INVALID_MEMSLOT;
    lpf_memslot_t dst_slot = LPF_INVALID_MEMSLOT;

    rc = lpf_register_global( _lpf, &localSlotCount, sizeof(size_t), &src_slot);
    EXPECT_EQ( "%d", LPF_SUCCESS, rc );
    rc = lpf_register_global( _lpf, globalSlotCounts, sizeof(size_t) * _size, &dst_slot );
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

    // globalSizes will hold exactly the union of all slot sizes at 
    // each process (zero or more) to become global.
    // That is, allgatherv is implemented here

    //start allgatherv for GlobalSizes
    size_t * globalSizes = static_cast<size_t *>(malloc(globalSlotCount * sizeof(size_t)));
    size_t * localSizes = (localSlotCount > 0) ? static_cast<size_t *>(malloc(localSlotCount * sizeof(size_t))) : nullptr;
    lpf_memslot_t slot_allgatherv_sizes = LPF_INVALID_MEMSLOT;
    rc = lpf_register_global( _lpf, globalSizes, globalSlotCount * sizeof(size_t), &slot_allgatherv_sizes);
    lpf_sync(_lpf, LPF_SYNC_DEFAULT);

    for (size_t i = 0; i < localSlotCount; i++) {
        //    for (size_t i=0, index = 0; i<_size; i++) {
        //      for (size_t j=0; j<globalSlotCounts[i]; j++,index++) {
        //if (i == _rank) {
        auto aLocalSlot = _pendingLocalToGlobalPromotions[tag][i].second;
        std::cout << "aLocalSlot = " << aLocalSlot << " from local slot i = " << i <<  std::endl;
        if (_localSlotMap.find(aLocalSlot) == _localSlotMap.end()) {
            std::cerr << "Error looking for locally registered slot!\n";
            std::abort();
        }
        LpfMemSlot promotedSlot =  _localSlotMap[aLocalSlot];
        localSizes[i] = promotedSlot.size;
        std::cout << "Setting slot size = " << i << " = " << localSizes[i] << std::endl;
        //}
        //}
        ////}
    }

    for (size_t i=0; i<localSlotCount; i++) {
        std::cout << "RANK " << _rank << " ->  localSizes[" << i << "] = " << localSizes[i] << std::endl;
    }

    /**
     * Copy my data data sizes to global array globalSizes
     */
    size_t allgatherv_start_addresses[_size];
    size_t allgatherv_lengths[_size];

    for (size_t i=0; i<_size; i++) allgatherv_start_addresses[i] = 0;
    for (size_t i=0; i<_size; i++) allgatherv_lengths[i] = 0;

    allgatherv_start_addresses[0] = 0;
    // the localSizes array already in bytes from register function!
    allgatherv_lengths[0] = localSizes[0];
    for (size_t i=1; i<_size; i++) {
        allgatherv_start_addresses[i] = allgatherv_start_addresses[i-1]+allgatherv_lengths[i-1];
        allgatherv_lengths[i] = localSizes[i];
    }

    for (size_t i=0; i<_size; i++) {
        std::cout << "RANK " << _rank << " ->  allgatherv_lengths[" << i << "] = " << allgatherv_lengths[i] << std::endl;
        std::cout << "RANK " << _rank << " ->  allgatherv_start_addresses[" << i << "] = " << allgatherv_start_addresses[i] << std::endl;
    }

    std::memcpy(&globalSizes[allgatherv_start_addresses[_rank]], localSizes, allgatherv_lengths[_rank]*sizeof(size_t));

    /**
     * ToDo: Use lpf_get variant whenever you fix 
     * lpf_get/sync combination
     *
     if (_rank != i) {
     lpf_get( _lpf, i, dst_slot2, index*sizeof(size_t), dst_slot2, index*sizeof(size_t), sizeof(size_t), LPF_MSG_DEFAULT);
     }
     */
    for (size_t i=0; i<_size; i++) {
        if ((i != _rank) && allgatherv_lengths[_rank] > 0) {
            std::cout << "Will put with size = " << allgatherv_lengths[_rank] << std::endl;
            lpf_put( _lpf, slot_allgatherv_sizes, allgatherv_start_addresses[_rank] /* offset */, i, slot_allgatherv_sizes, allgatherv_start_addresses[_rank], allgatherv_lengths[_rank], LPF_MSG_DEFAULT);
        }
    }
    lpf_sync(_lpf, LPF_SYNC_DEFAULT);
    //end allgatherv for globalSizes
    
    lpf_deregister(_lpf, slot_allgatherv_sizes);

    for (size_t i=0; i<globalSlotCount; i++) {
        std::cout << "RANK " << _rank << " ->  globalSizes[" << i << "] = " << globalSizes[i] << std::endl;
    }

    //for (size_t i=0, index=0; i<_size; i++) {
        // Make local slots of rank i global !!!
    for (size_t i=0; i<_size; i++) {

        for (size_t j=allgatherv_start_addresses[i]; j<allgatherv_start_addresses[i]+allgatherv_lengths[i]; j++) {
            lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
            void * newBuffer = nullptr;
            size_t newSize = globalSizes[j];
            /**
             * only allocate local buffers where needed
             * that is: where the rank is promoting its local
             * slot to a global slot
             */
            if (i == _rank) {
                auto aLocalSlot = _pendingLocalToGlobalPromotions[tag][j-allgatherv_start_addresses[j]].second;
                if (_localSlotMap.find(aLocalSlot) == _localSlotMap.end()) {
                    std::cout << "FIRST: " << _pendingLocalToGlobalPromotions[tag][j-allgatherv_start_addresses[j]].first << std::endl;
                    std::cout << "SECOND: " <<_pendingLocalToGlobalPromotions[tag][j-allgatherv_start_addresses[j]].second << std::endl;
                    std::cerr << "Could not find a locally registered slot, exiting\n";
                    std::abort();
                }
                LpfMemSlot promotedSlot = _localSlotMap[aLocalSlot];
                newBuffer = promotedSlot.buffer;
                // local memslot can be locally deregistered
                std::cout << "Rank " << _rank << " Try to deregister slot with key " << aLocalSlot << std::endl;
                rc = lpf_deregister(_lpf, promotedSlot.lpfSlot);
                EXPECT_EQ( "%d", LPF_SUCCESS, rc );
                // remove local slot from local slot hash table
                _localSlotMap.erase(aLocalSlot);
            }
            else {
                newBuffer = malloc(newSize);
            }

            std::cout << "Rank " << _rank << " Will register globally a slot of size " << newSize << std::endl;
            rc = lpf_register_global(_lpf, newBuffer, newSize, &newSlot);
            lpf_sync(_lpf, LPF_SYNC_DEFAULT);
            EXPECT_EQ( "%d", LPF_SUCCESS, rc );
            auto tag = _currentTagId++;
            /**
             * It is essential to record who this memory slot
             * belongs to (process i). Without this record, future data
             * movement calls will not work. This is recorded in the
             * targetRank of the destination slot.
             */
            size_t remote = i;
            _globalSlotMap[tag] = LpfMemSlot {.lpfSlot = newSlot, .size = newSize, .buffer = newBuffer, .targetRank = remote};
            //globalSlotIds.push_back(tag);
            hicr2LpfSlotMap[tag] = newSlot;
        } 
    }
    //}

    size_t msg_cnt;
    for (auto i : _globalSlotMap) {
        lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, i.second.lpfSlot);
        hicrSlotId2MsgCnt[i.first] = msg_cnt;
    }

    free(globalSlotCounts);
    free(globalSizes);
    //return globalSlotIds;
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
    if ( _globalSlotMap.find(destination) == _globalSlotMap.end()) {
      std::cerr << "Destination slot: Cannot find entry in global slot map\n";
      std::abort();
    }
    else {
      dstSlot =  _globalSlotMap[destination];
    }

    LpfMemSlot srcSlot;
    if (_globalSlotMap.find(source) == _globalSlotMap.end()) {
      if (_localSlotMap.find(source) == _localSlotMap.end()) {
        std::cerr << "Source slot: Cannot find entry in local or global slot Map\n";
        std::abort();
      }
      else {
        srcSlot = _localSlotMap[source];
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
        _localSlotMap[memorySlotId] = LpfMemSlot {.lpfSlot = lpfSlot, .size = size, .buffer = addr};
        std::cout << "Rank " << _rank << " registered a slot with slotId " << memorySlotId << std::endl;
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
