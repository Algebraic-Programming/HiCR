
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file objectStore.hpp
 * @brief Provides functionality for a block object store over HiCR
 * @author A. N. Yzelman
 * @date 29/11/2023
 */

#pragma once

namespace HiCR
{

/**
 * Encapsulates the concept of a block in the object store.
 *
 * A block is a globally unique reference to a memory region that exists
 * somewhere on the system. It is created via a call to #publish.
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
 * While for non-owners, the memory block is read-only, the block object store
 * does support changing ownership. In this case, a current owner must indicate
 * its willingness to abdicate ownership via a call to #destroyOrRelease, while
 * at least one non-owner worker must indicate its willingness to take over
 * ownership via a call to #tryGetOwnership. If no such worker exist, the block
 * will be destroyed instead.
 *
 * A call to #publish is not collective, meaning that remote workers will not
 * immediately be able to refer to any remotely published block. For remote
 * workers to be able to refer to a block, two things should happen first:
 *  -# the worker owner and the workers who refer to that block, should
 *     issue a collective call to #update; and
 *  -# the block returned by #publish should be copied to the remote workers
 *     that want to make use of it (e.g., by using channels or raw memory
 *     copies).
 * Therefore, instances of the #Block type shall be copyable, in the exact sense
 * of the corresponding C++ type trait. This informally means it can both be the
 * target for raw memcpy calls as well as the target for inter-worker
 * communication. Copying allows for remote workers to refer to blocks it did
 * not (initially) own.
 *
 * If a block has been copied between workers but no #update has occurred yet,
 * it will be illegal for non-owner workers to call #get or #tryGetOwnership.
 * Any other call on the block will be legal if their individual conditions are
 * met (e.g., calls to #destroyOrRelease will be legal but fail for non-owners,
 * while a call to #getByteSize always succeeds).
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
 * Ownership of a block can be changed via a call to #tryGetOwnership. This may
 * or may not fail. If successful, the change in ownership will be effected at
 * the end of a subsequent call to #update-- i.e., the #update, in addition to
 * synchronising published block index data, also dubs as the point of
 * consistency in any ownership change.
 *
 * For ownership of a block to change during a call to #update, the following
 * conditions must be met:
 *  -# the owner has issued a call to #destroyOrRelease before this call to
 *     #update. Earlier calls to #destroyOrRelease (meaning, before a
 *     \em previous call to #update) shall \em not satisfy this condition;
 *  -# at least one non-owner worker has issued a call to #tryGetOwnership
 *     before this call to #update. Earlier calls to #tryGetOwnership (meaning,
 *     before a previous call to #update) shall \em not satisfy this condition.
 * If there are multiple non-owners attempting to attain ownership of a block,
 * at most one of them will be succesful. If any one of the conditions is not
 * satisfied, ownership shall not change.
 *
 * Ownership status of a block may be inspected via a call to #isOwner, which
 * thus allows an owner to check if it has lost ownership of a given block and
 * allows a non-owner to check if it has won ownership of a given block.
 *
 * A call to #update is a blocking and collective function. It does not have an
 * asynchronous counterpart.
 *
 * A call to #fence is a blocking function. A non-blocking variant is provided
 * by #test_fence.
 *
 * For performance safety, all calls to #get and #fence require a tag. A tag is
 * created via the unified machine model. Tags allow grouping communication of
 * multiple blocks, and incur message latency only once on the whole group,
 * rather than multiple times for each message in the group.
 */
class Block
{
  public:

  /**
   * Creates a new, globally unique data block from an existing pointer and
   * byte size pair.
   *
   * @param[in] ptr  The pointer to the memory region to be published.
   * @param[in] size The size (in bytes) of the memory region \a ptr points
   *                 to.
   *
   * @returns A block reference to (\a ptr, \a size).
   *
   * The calling worker will be the owner of the returned block.
   *
   * If other workers are to refer to returned block, then 1) those
   * workers and the caller worker to this function must be included with
   * a call to #update, \em and 2) the block should be copied to such remote
   * workers. Use, for example, the channel or the raw datamover
   * (#memcpy) for this purpose.
   *
   * A call to this function entails \f$ \Theta(1) \f$ work. It is (thus) a
   * non-blocking, asynchronous call: registration with remote workers may
   * occur at any time between this call and the end of a subsequent call to
   * #update that includes this worker.
   *
   * A call to this function is \em not thread-safe.
   */
  static Block publish(void *const ptr, size_t size);

  /**
   * Fetches all block meta-data published by all workers within a set of
   * instances.
   *
   * @tparam FwdIt A forward iterator over a collection of instances.
   *
   * @param[in] start The start iterator over the collection.
   * @param[in] end   Iterator over the collection that is in end-position.
   *
   * This is a blocking and collective call across all workers in the given
   * collection. A call to this function implies a #fence-- that is, after a
   * call to this function, any pending calls to #get will have been completed
   * for all instances with owning and retrieving workers in the collection of
   * instances in \a start to \a end.
   *
   * The collection of instances must match across all instances participating
   * in this collective call to #update. If there are multiple calls to #update
   * at any of the involved workers, then for those calls to #update that have
   * a non-empty intersection of workers, the order of calls should match.
   *
   * A call to this function is expensive even if #publish is implemented in
   * an eager fashion, as the update even in the optimistic case where the
   * registration has completed across the entire system, still requires a
   * subset barrier (the system needs to know everyone has a consistent
   * index).
   *
   * \warning Hence, use this function as sparingly as possible.
   *
   * A call to this function shall \em not be thread-safe.
   */
  template <typename FwdIt>
  static void update(FwdIt start, const FwdIt &end);

  /**
   * Fences all block activity attached to a given \a tag.
   *
   * @param[in] tag Guarantees resolving all calls to #get with the given
   *                \a tag.
   *
   * This is a blocking call. All workers that have communications in the
   * given \a tag must make the same number of calls to #fence. This
   * (paradoxically) allows for fully asynchronous fencing.
   *
   * \warning I.e., a call to fence on one party may exit before a matching
   *          call elsewhere has finished: a call to this function does
   *          \em not imply a barrier.
   *
   * A call to this function is \em not thread-safe.
   */
  static void fence(const Tag &tag);

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

  /**
   * Retrieve a pointer to the block contents.
   *
   * @param[in] tag The tag associated with this request.
   *
   * @returns Pointer to the local memory that mirrors the block owner's
   *          content.
   *
   * The contents are guaranteed to match with the owner's after the next call
   * to #fence over the given \a tag.
   *
   * \note For the owner of the block, this corresponds to a pointer to the
   *       memory area corresponding to the true block data.
   *
   * A call to this function implies \f$ \mathcal{O}(n) \f$ runtime, where
   * \f$ n \f$ is the byte size of this block.
   *
   * \note The intent is that this upper bound on work occurs at most once
   *       per block, and only in the case that retrieval first requires the
   *       allocation of a local buffer.
   *
   * A call to this function is non-blocking, since the contents of the memory
   * the return value points to are undefined until the next call to #fence
   * with the given \a tag. This function is one-sided and (thus) \em not
   * collective.
   *
   * Repeated calls to this function with the same \a tag but without any
   * intermediate calls to #fence with that \a tag will behave as though only
   * a single call was made.
   *
   * A call to this function shall be thread-safe.
   *
   * \note The thread-safety plus re-entrant-safety means the implementation
   *       of this function must include an atomic guard that is unlucked at
   *       a call to #fence.
   */
  const void *get(const Tag &tag);

  /**
   * Queries whether the calling worker is the owner of this block.
   *
   * @returns Whether this worker has ownership of this block.
   *
   * Ownership is guaranteed immutable until the next call to #update.
   *
   * A call to this function shall incur \f$ \mathcal{O}(1) \f$ work, is
   * thread-safe, and shall never fail.
   */
  bool isOwner() const noexcept;

  /**
   * Queries the byte size of this block.
   *
   * @returns The size of this block, in bytes.
   *
   * A call to this function shall incur \f$ \mathcal{O}(1) \f$ work, is
   * thread-safe, and shall never fail.
   */
  const size_t getByteSize() const noexcept;

  /**
   * The owner allows electing another worker as the owner of the block, and
   * if no other worker takes over ownership, will destroy the block.
   *
   * The destruction of the block or the change of ownership will only take
   * effect after a subsequent call to #update.
   *
   * \warning If the block is destroyed, any local associated memory will
   *          \em not be freed by the unified layer.
   *
   * Ownership would be transferred to another worker if:
   *  -# the subsequent #update had at least one other worker who called
   *     #tryGetOwnership on this block.
   *
   * The block would instead be destroyed if:
   *  -# the subsequent #update had no other worker who called
   *     #tryGetOwnership on this block.
   *
   * @throws An logic exception if called by a worker that is not the owner
   *         of this block.
   *
   * This is a non-blocking asynchronous function that incurs
   * \f$ \mathcal{O}(1) \f$ work.
   *
   * Repeated calls to this function without any intermediate calls to #fence
   * will behave as though only a single call was made.
   *
   * A call to this function shall be thread-safe.
   */
  void destroyOrRelease();

  /**
   * This worker attempts, between now and the end of a subsequent call to
   * #update, to obtain ownership of this block.
   *
   * If successful, the contents of the memory block returned by #get will
   * correspond to the true value of this block; future calls to #get made by
   * other workers will then refer to this worker.
   *
   * If before the next call to #update there was no call to #get, then any
   * changes in the block by the previous owner shall be lost.
   *
   * If both this function as well as #get are called before the next call to
   * #update \em and the attempt to retrieve ownership is successful, then the
   * contents of the block are guaranteed to be consistent between all of 1)
   * the previous owner, 2) the new owner, i.e., this worker, and 3) any
   * potential third-worker calls to #get that follow the corresponding call
   * to #update.
   *
   * @throws An logic exception if called by a worker that is already the
   *         owner of this block.
   *
   * This is a non-blocking and asynchronous operation that entails
   * \f$ \mathcal{O}(1) \f$ work.
   *
   * Repeated calls to this function without any intermediate calls to #fence
   * will behave as though only a single call was made.
   *
   * A call to this function shall be thread-safe.
   */
  void tryGetOwnership();

  private:

  void *_objectPointer;

  size_t _objectSize;

  // worker_id_t  _ownerId;
};

} // end namespace HiCR
