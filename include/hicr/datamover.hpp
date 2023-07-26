
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

namespace HiCR {

/**
 * Encapsulates a memory region that is either the source or destination of a
 * call to HiCR::memcpy.
 *
 * There are a limited number of memory slots supported by any system.
 */
class MemorySlot {
 public:
  /**
   * Constructs a memory slot.
   *
   * @param[in] addr  The start address of the memory region to be registered.
   * @param[in] size  The size of the memory region to be registered.
   * @param[in] local Whether this is a local or global registration.
   *
   * Global registrations must be done collectively by all localities.
   */
  MemorySlot(void * addr, const size_t size, const bool local);
};

/**
 * A message tag.
 *
 * For asynchronous data movement, fences may operate on messages that share the
 * same tag; meaning, that while fencing on a single message or on a group of
 * messages that share a tag, other messages may remain in flight after the
 * fence completes.
 *
 * There are a limited set of tags exposed by the system.
 */
class TagSlot {
 public:
  /**
   * Constructs a new message and fence tag.
   *
   * There are a limited set of tags exposed by the system.
   *
   * @param[in] name The name of the tag.
   */
  TagSlot(const std::string& name);
}

/**
 * A set of localities.
 */
class LocalitySet {
 public:
  /**
   * Iterates over a set of locality IDs and adds them to the set.
   */
  template< typename ContainerIterator >
  LocalitySet( ContainerIterator& start, ContainerIterator& end );
}

/**
 * Performs an asynchronous memory copy from within a source area, to within a
 * destination area.
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
 * Either \a source and \a src_locality, or \a destination and \a dst_locality,
 * \em must refer to a local memory region.
 *
 * A call to this function is one-sided, non-blocking, and, if the hardware and
 * network supports it, zero-copy.
 *
 * \note For blocking semantics, simply immediately follow this call to memcpy
 *       with a call to HiCR::fence().
 */
void memcpy(
 const MemorySlot& source, const size_t src_locality, const size_t src_offset,
 MemorySlot& destination, const size_t dst_locality, const size_t dst_offset,
 const size_t size,
 const TagSlot& tag
);

/**
 * Fences a group of memory copies.
 *
 * \warning This fence implies a (non-blocking) all-to-all across the entire
 *          system. While this latency can be hidden by yielding plus a
 *          completion event, the latency exists nonetheless. If, therefore,
 *          the latency cannot be hidden, then zero-cost fencing (see below)
 *          should be employed instead.
 */
void fence(const TagSlot& tag);

/**
 * Fences a group of memory copies using zero-cost synchronisation.
 *
 * This fence does \em not incur any all-to-all collective as part of the
 * synchronisation.
 *
 * @param[in] tag      The tag of the memory copies to fence for
 * @param[in] msgs_in  How many messages are incoming to this locality
 * @param[in] msgs_out How many messages are outgoing from this locality
 * @param[in] sources  A set of localities from which we have incoming memory
 *                     copy requests
 */
void fence(
 const TagSlot& tag,
 const size_t msgs_in, const size_t msgs_out,
 const LocalitySet& sources
);

