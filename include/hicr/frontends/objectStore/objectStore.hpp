
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
