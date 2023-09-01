/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file backend.hpp
 * @brief Provides a definition for the backend class.
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>
#include <hicr/computeResource.hpp>
#include <hicr/memorySpace.hpp>

namespace HiCR
{

class Runtime;

/**
 * Common definition of a collection of compute resources
 */
typedef std::vector<std::unique_ptr<ComputeResource>> computeResourceList_t;

/**
 * Common definition of a collection of memory spaces
 */

// typedef std::vector<std::unique_ptr<MemorySpace>> memorySpaceList_t;
typedef std::vector<MemorySpace *> memorySpaceList_t;

/**
 * Encapsulates a HiCR Backend.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform common operations on the supported device/network library.
 *
 */
class Backend
{
  friend class Runtime;

  protected:

  Backend() = default;

  public:

  virtual ~Backend() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  __USED__ virtual void queryResources() = 0;

  /**
   * This function returns the list of queried compute resources, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of compute resources, as detected the last time \a queryResources was executed.
   */
  __USED__ inline computeResourceList_t &getComputeResourceList() { return _computeResourceList; }

  /**
   * This function returns the list of queried memory spaces, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of memory spaces, as detected the last time \a queryResources was executed.
   */
  __USED__ inline memorySpaceList_t &getMemorySpaceList() { return _memorySpaceList; }

  /**
   * Instructs the backend to perform an asynchronous memory copy from
   * within a source area, to within a destination area.
   *
   * @param[in] source       The source memory region
   * @param[in] src_locality The locality of the source memory region
   * @param[in] src_offset   The offset (in bytes) within \a source at
   *                         \a src_locality
   * @param[in] destination  The destination memory region
   * @param[in] dst_locality The locality of the destination memory region
   * @param[in] dst_offset   The offset (in bytes) within \a destination at
   *                         \a dst_locality
   * @param[in] size         The number of bytes to copy from the source to the
   *                         destination
   * @param[in] tag          The tag of this memory copy
   *
   * A call to this function is one-sided, non-blocking, and, if the hardware and
   * network supports it, zero-copy.
   *
   * If there is no direct path of communication possible between the memory
   * spaces that underlie \a source and \a destination (and their localities), an
   * exception will be thrown.
   *
   * The memory spaces (localities) that \a source and \a destination have been
   * created with must exist within the memory spaces \a tag was created with.
   *
   * \note The reverse (the memory spaces \a tag was created with must exist
   *       within those that \a source or \a destination have been created with)
   *       need \em not be true; the set of memory spaces corresponding to the
   *       \a tag are a superset of the union of those corresponding to \a source
   *       and \a destination.
   *
   * \note If \a source is a local memory slot, then \a src_locality \em must be
   *       <tt>0</tt>-- a local memory slot only has its own locality.
   *
   * \note If \a destination is a local memory slot, then \a src_locality \em must
   *       be <tt>0</tt>-- a local memory slot only has its own locality.
   *
   * \note For blocking semantics, simply immediately follow this call to memcpy
   *       with a call any of the HiCR::wait() variants. If you would like a
   *       blocking memcpy, we can provide a small library that wraps this
   *       function with a wait. While this would perhaps be easier to use, it
   *       requires two-sided interaction (in case a remote memory space is
   *       involved) \em and will likely wreak havoc on performance of the upper-
   *       level run-time system.
   *
   * \internal Since memory slots are tied to memory spaces, a sparse matrix
   *           \f$ M \f$ internal to HiCR prescribes which backends can facilitate
   *           data movement between pairs of memory spaces. This memcpy hence
   *           looks into this table \f$ M \f$, picks the right backend mechanism,
   *           and translates the memcpy call into that backend-specific call.
   *
   * Exceptions are thrown in the following cases:
   *  -# HiCR cannot facilitate communication between the requested memory spaces;
   *  -# The offset and sizes result in a communication request that is outside
   *     the region of either \a destination or \a source;
   *  -# Both \a destination and \a source are local memory slots;
   *  -# One (or both) of \a dst_locality and \a src_locality point to
   *     non-existing memory spaces;
   *  -# \a dst_locality is a local memory slot but \a dst_locality is not 0;
   *  -# \a src_locality is a local memory slot but \a src_locality is not 0;
   *  -# the localities in \a tag are not a superset of the localities registered
   *     with \a source or \a destination.
   *
   * \todo Should this be <tt>nb_memcpy</tt> to make clear that, quite different
   *       from the NIX standard <tt>memcpy</tt>, it is nonblocking?
   */
  virtual void nb_memcpy(
    MemorySlot &destination,
    const size_t dst_locality,
    const size_t dst_offset,
    const MemorySlot &source,
    const size_t src_locality,
    const size_t src_offset,
    const size_t size,
    const Tag &tag) = 0;

  /**
   * Fences a group of memory copies.
   *
   * This is a collective and blocking call; returning from this function
   * indicates that all local incoming memory movement has completed \em and that
   * all outgoing memory movement has left the local interface (and is guaranteed
   * to arrive at the remote memory space, modulo any fatal exception).
   *
   * @param[in] tag The tag of the memory copies to wait for.
   *
   * \warning This wait implies a (non-blocking) all-to-all across all memory
   *          spaces the \a tag was created with. While this latency can be hidden
   *          by yielding plus a completion event or by polling (see test_wait),
   *          the latency exists nonetheless. If, therefore, this latency cannot
   *          be hidden, then zero-cost fencing (see below) should be employed
   *          instead.
   *
   * \note While the wait is blocking, local successful completion does \em not
   *       guarantee that all other memory spaces the given \a tag has been
   *       created with, are done with their local wait.
   *
   * Exceptions are thrown in the following cases:
   *  -# One of the remote address spaces no longer has an active communication
   *     channel. This is a fatal exception from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   *
   * \todo How does this interact with malleability of resources of which HiCR is
   *       aware? One possible answer is a special event that if left unhandled,
   *       is promoted to a fatal exception.
   */
  virtual void wait(const Tag &tag) = 0;

  /**
   * Fences a group of memory copies using zero-cost synchronisation.
   *
   * This is a collective and blocking call; returning from this function
   * indicates that all local incoming memory movement has completed \em and that
   * all outgoing memory movement has left the local interface (and is guaranteed
   * to arrive at the remote memory space, modulo any fatal exception).
   *
   * This wait does \em not incur any all-to-all collective as part of the
   * operation; one way to look at this variant is that all meta-data an
   * all-to-all collects in the other wait variant here is provided as arguments
   * instead.
   *
   * @param[in] tag      The tag of the memory copies to wait for.
   * @param[in] msgs_out How many messages are outgoing from this locality.
   * @param[in] msgs_in  How many messages are incoming to this locality.
   * @param[in] dests    A stard iterator over localities to which we have outgoing
   *                     memory copy requests.
   * @param[in] dests_end  An end iterator over localities to which we have outgoing
   *                       memory copy requests.
   * @param[in] sources  A start iterator over localities from which we have incoming
   *                     memory copy requests.
   * @param[in] sources_end  An end iterator over localities from which we have incoming
   *                     memory copy requests.
   *
   * \note A remote worker may initiate a memory copy where the source is local
   *       to us. Even though the memory copy was not issued by us, this still
   *       counts as one outgoing message and should be accounted for by
   *       \a msgs_out and should lead to an entry iterated over by \a dests.
   *
   * \note A remote worker may initiate a memory copy where the destination is
   *       local to us. Even though the memory copy was not issued by us, this
   *       still counts as one incoming message and should be accounted for by
   *       \a msgs_in and should lead to an entry iterated over by \a sources.
   *
   * Exceptions are thrown in the following cases:
   *  -# An entry of \a dests or \a sources is out of range w.r.t. the localities
   *     \a tag has been created with.
   *  -# One of the remote address spaces no longer has an active communication
   *     channel. This is a critical failure from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   *
   * \note One difference between the global wait is that this variant
   *       potentially may \em not detect remote failures of resources that are
   *       not tied to those in \a dests or \a sources; the execution may proceed
   *       without triggering any exception, until such point another wait waits
   *       for communication from a failed resource.
   *
   * \todo How does this interact with malleability of resources of which HiCR is
   *       aware? One possible answer is a special exception / event.
   */
  template <typename DstIt = const size_t *, typename SrcIt = const size_t *>
  void wait(
    const Tag &tag,
    const size_t msgs_out,
    const size_t msgs_in,
    DstIt dests,
    const DstIt dests_end,
    SrcIt sources,
    const SrcIt sources_end);

  /**
   * A nonblocking wait that returns whether the wait is completed.
   *
   * @param[in] tag      The tag of the memory copies to wait for.
   * @param[in] msgs_out How many messages are outgoing from this locality.
   * @param[in] msgs_in  How many messages are incoming to this locality.
   * @param[in] dests    A stard iterator over localities to which we have outgoing
   *                     memory copy requests.
   * @param[in] dests_end  An end iterator over localities to which we have outgoing
   *                       memory copy requests.
   * @param[in] sources  A start iterator over localities from which we have incoming
   *                     memory copy requests.
   * @param[in] sources_end  An end iterator over localities from which we have incoming
   *                     memory copy requests.
   *
   * @returns <tt>false</tt> if the wait has not yet completed.
   * @returns <tt>true</tt> if the wait has completed.
   *
   * A call to this function that returns <tt>true</tt> dubs as satisfying the
   * collective requirement of calls to #wait; no further call to #wait is
   * required.
   *
   * \warning First calling #test_wait that returns <tt>true</tt> and \em then
   *          calling a regular #wait results in the latter waiting for a second
   *          wait.
   *
   * The arguments of the test mirror that of the zero-cost wait. This is because
   * otherwise first an all-to-all meta-data exchange would have to be completed
   * before we may start \f$ \Theta(1) \f$ polling.
   *
   * \todo A non-zero-cost test_wait may be provided by the first call initiating
   *       a non-blocking all-to-all on the meta-data. But let us first see if a
   *       compelling use case for this exists, as the initiation of an all-to-all
   *       seems contrary to the purposes of being able to poll the status of all
   *       communications corresponding to a tag (even if the all-to-all is
   *       non-blocking, it can only progress if all workers hit the same call to
   *       test_wait, which in fact also entails defining collective \em like
   *       semantics for test_wait (all involved memory spaces must execute
   *       test_wait at least once, or at least twice if the first call returned
   *       <tt>false</tt>).
   *
   * A call to #test_wait that returns <tt>true</tt> may be matched on remote
   * memory spaces with a blocking call to the zero-cost variant of #wait.
   *
   * Exceptions are thrown in the following cases:
   *  -# An entry of \a dests or \a sources is out of range w.r.t. the localities
   *     \a tag has been created with. This exception only triggers on the first
   *     call to #test_wait. Backends are not required to check that subsequent
   *     calls retain valid arguments.
   *  -# One of the remote address spaces no longer has an active communication
   *     channel. This is a critical failure from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   */
  template <typename DstIt = const size_t *, typename SrcIt = const size_t *>
  bool test_wait(
    const Tag &tag,
    const size_t msgs_out,
    const size_t msgs_in,
    DstIt dests,
    const DstIt dests_end,
    SrcIt sources,
    const SrcIt sources_end);

  /**
   * A blocking wait that waits on any tag that has previously unsuccessfully
   * been tested for using a call to #test_wait that returned <tt>false</tt>.
   *
   * @tparam FwdIt A forward iterator over #HiCR::Tag instances.
   *
   * @param[in] tags An iterator over a set of tags that previously have been
   *                 provided to #test_wait, which returned <tt>false</tt> on its
   *                 last such invocation, and have since 1) not been provided to
   *                 a call to #wait or 2) returned by an earlier call to
   *                 #wait_any.
   *
   * @param[in] tags_end An iterator that indicates the end position of \a tags.
   *
   * The iterator pair \a tags and \a tags_end must iterate over at least one tag.
   *
   * @returns The tag of a group of messages that has completed. The call to this
   *          function then counts as fulfilling the collective requirement of the
   *          zero-cost #wait on the returned tag. The returned tag is an entry
   *          that \a tags could iterate over.
   *
   * \warning First calling #wait_any and \em then calling a regular #wait on
   *          the returned tag results in the latter waiting for a second wait.
   *
   * After completion of the indicated group of messages, there is no guarantee
   * that any of other tags have all its messages completed as well-- subsequent
   * calls to #wait_any or #test_wait would be required.
   *
   * For example, if there is a container \f$ S \f$ that holds three tags
   * \f$ S = \{ a, b, c \} \f$, then the following sequence of calls would
   * first test for the completion of the tags, then do some unrelated work, and
   * when that completes, wait for any of the tags previously unsuccessfully
   * tested for:
   *
   * \code
   * // ...
   *
   * std::set< Tag > incomplete;
   * for( const Tag& t : S ) {
   *   if( test_wait( t, out[t], in[t], D[t], D_end[t], S[t], D_end[t] ) ) {
   *      // trigger computation related to t
   *   } else {
   *      incomplete.insert( t );
   *   }
   * }
   *
   * // ...
   * // do work unrelated to a, b, c
   * // ...
   *
   * while( incomplete.size() > 0 ) {
   *   const Tag t = wait_any( incomplete.cbegin(), incomplete.cend() );
   *   // trigger computation related to t
   *   incomplete.erase( t );
   * }
   *
   * // ...
   * \endcode
   *
   * Exceptions are thrown in the following cases:
   *  -# the collection that \a tags and \a tags_end span is empty;
   *  -# a tag that \a tags iterates over did not have a prior unsuccessful call
   *     to #test_wait or since had been 1) the argument to a call to #wait,
   *     or 2) had been returned by a call to #wait_any;
   *  -# one of the remote address spaces no longer has an active communication
   *     channel. This is a critical failure from which HiCR cannot recover. The
   *     user is encouraged to exit gracefully without initiating any further
   *     communication or waits.
   *
   * \internal This specification requires that HiCR internally maintains a list
   *           of tags and corresponding meta-data, the latter of which will be
   *           initialised by the first call to #test_wait and only if that call
   *           returned <tt>false</tt>.
   */
  template <typename FwdIt = const Tag *>
  Tag wait_any(FwdIt tags, const FwdIt tags_end);

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  /**
   * The internal container for the queried resources.
   */
  computeResourceList_t _computeResourceList;

  /**
   * The internal container for the queried resources.
   */
  memorySpaceList_t _memorySpaceList;
};

} // namespace HiCR
