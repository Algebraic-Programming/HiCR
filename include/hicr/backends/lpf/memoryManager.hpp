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
#include <lpf/core.h>
#include <lpf/collectives.h>
#include <hicr/backends/memoryManager.hpp>
#include <hicr/backends/lpf/memorySlot.hpp>

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

class MemoryManager final : public HiCR::backend::MemoryManager
{

  private:

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
  /**
   * @initMsgCnt is a map from a HiCR slot ID
   * to the initial message count. This count may not be zero,
   * as slots can get reused and reassigned in LPF.
   */
  std::map<MemorySlot, size_t> initMsgCnt;

  /**
   * The decision to resize memory register in the constructor
   * is because this call requires lpf_sync to become effective.
   * This makes it almost impossible to do local memory registrations with LPF.
   * On the other hand, the resize message queue could also be locally
   * made, and placed elsewhere.
   */
  MemoryManager(size_t size, size_t rank, lpf_t lpf) : HiCR::backend::MemoryManager(), _size(size), _rank(rank), _lpf(lpf) 
    {

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

  void queryResources() {}

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
      // Obtaining local slots to exchange
      size_t localSlotCount = memorySlots.size();

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
      std::vector<globalKey_t> localSlotKeys(localSlotCount);
      std::vector<size_t> localSlotProcessId(localSlotCount);
      std::vector<size_t> globalSlotSizes(globalSlotCount);
      std::vector<globalKey_t> globalSlotKeys(globalSlotCount);
      std::vector<void *> globalSlotPointers(globalSlotCount);
      std::vector<size_t> globalSlotProcessId(globalSlotCount);

      for (size_t i = 0; i < localSlotCount; i++) {
          const auto key = memorySlots[i].first;
          const auto memorySlot = memorySlots[i].second;
          localSlotSizes[i] = memorySlot->getSize();
          localSlotKeys[i] = key;
          localSlotProcessId[i] = _rank;
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
              // deregister locally as it will be registered globally

              lpf::MemorySlot * memorySlot = static_cast<lpf::MemorySlot *>(memorySlots[localPointerPos++].second);
              lpf_deregister(_lpf, memorySlot->getLPFSlot());
              globalSlotPointers[i] = memorySlot->getPointer();
              // optionally initialize here?
          }


          lpf_memslot_t newSlot = LPF_INVALID_MEMSLOT;
          rc = lpf_register_global(_lpf, globalSlotPointers[i], globalSlotSizes[i], &newSlot);

          // Creating new memory slot object
          auto memorySlot = new lpf::MemorySlot(
                  globalSlotProcessId[i],
                  newSlot,
                  globalSlotPointers[i],
                  globalSlotSizes[i],
                  tag,
                  globalSlotKeys[i]);

          lpf_sync(_lpf, LPF_SYNC_DEFAULT);
          EXPECT_EQ( "%d", LPF_SUCCESS, rc );
          //auto tag = _currentTagId++;
          /**
           * It is essential to record who this memory slot
           * belongs to (process i). Without this record, future data
           * movement calls will not work. This is recorded in the
           * targetRank of the destination slot.
           */
          // Registering global slot

          // Registering global slot
          
          size_t msg_cnt;
          lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, memorySlot->getLPFSlot());
          initMsgCnt[*memorySlot] = msg_cnt;

          registerGlobalMemorySlot(memorySlot);
      } 

  }

  __USED__ inline void memcpyImpl(HiCR::MemorySlot *destinationSlotPtr, const size_t dstOffset, HiCR::MemorySlot *sourceSlotPtr, const size_t srcOffset, const size_t size) override
  {

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

    // Getting up-casted pointer for the execution unit
    auto destination = dynamic_cast<MemorySlot *>(destinationSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (destination == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting up-casted pointer for the execution unit
    auto source = dynamic_cast<MemorySlot *>(sourceSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto sourceRank = source->getRank();
    const auto destinationRank = destination->getRank();

    bool isSourceRemote = sourceRank != _rank;
    bool isDestinationRemote = destinationRank != _rank;

    // Checking whether source and/or remote are remote

    // Sanity checks
    if (isSourceRemote && isDestinationRemote) HICR_THROW_LOGIC("Trying to use LPF backend perform a remote to remote copy between slots");

    //if (isSourceRemote && isDestinationGlobalSlot == false) HICR_THROW_LOGIC("Trying to use MPI backend in remote operation to with a destination slot(%lu) that has not been registered as global.", destination);

    // Calculating pointers
    lpf_memslot_t dstSlot = destination->getLPFSlot();
    lpf_memslot_t srcSlot = source->getLPFSlot();


    // Perform a get if the source is remote and destination is local
    if (isSourceRemote && !isDestinationRemote)
    {
        lpf_get( _lpf, srcSlot, srcOffset, destination->getRank(), dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
    }
    // Perform a put if source is local and destination is global
    else if (!isSourceRemote && isDestinationRemote)
    {
        lpf_put( _lpf, srcSlot, srcOffset, destination->getRank(), dstSlot, dstOffset, size, LPF_MSG_DEFAULT);
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

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   *
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
   lpf_memslot_t lpfSlot = LPF_INVALID_MEMSLOT;
   auto rc = lpf_register_local( _lpf, ptr, size, &lpfSlot );
   if (rc != LPF_SUCCESS) HICR_THROW_RUNTIME("LPF Memory Manager: lpf_register_local failed");

    // Creating new memory slot object
    auto memorySlot = new MemorySlot(_rank, lpfSlot, ptr, size);
    return memorySlot;
  }

  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::MemorySlot *memorySlot) override
  {
      pullMessagesRecv(memorySlot);
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::MemorySlot *memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_cast<MemorySlot *>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    lpf_deregister(_lpf, memorySlot->getLPFSlot());

  }

  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void freeLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory freeing");
  }

  __USED__ inline HiCR::MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
      HICR_THROW_RUNTIME("This backend provides no support for memory allocation");
  }

    __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
    {
        // No memory spaces are provided by this backend
        return memorySpaceList_t({});
    }
//    memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpaceId, const size_t size)  {
//        
//        memorySlotId_t m = 0; return m;}


  __USED__ inline void pullMessagesRecv(HiCR::MemorySlot * memorySlot) {

      size_t msg_cnt;
      lpf::MemorySlot * memSlot = static_cast<lpf::MemorySlot *>(memorySlot);
      lpf_memslot_t lpfSlot = memSlot->getLPFSlot();
      lpf_get_rcvd_msg_count_per_slot(_lpf, &msg_cnt, lpfSlot);
      for (size_t i = initMsgCnt[*memSlot] + memSlot->getMessagesRecv(); i < msg_cnt; i++) 
          memSlot->increaseMessagesRecv();

  }

    __USED__ inline void flush() override {
        lpf_flush(_lpf);
    }

};

} // namespace lpf
} // namespace backend
} // namespace HiCR
