/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file messageTag.hpp
 * @brief Provides a definition for the memory tag class.
 * @author A. N. Yzelman
 * @date 8/8/2023
 */

#pragma once

#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Definition for a 128 bit number
 */
typedef std::pair<uint64_t, uint64_t> uint128_t;

/**
 * Encapsulates a message tag.
 *
 * For asynchronous data movement, fences may operate on messages that share the
 * same tag; meaning, that while fencing on a single message or on a group of
 * messages that share a tag, other messages may remain in flight after the
 * fence completes.
 *
 * There are a limited set of tags exposed by the system.
 *
 * The Tag may be memcpy'd between HiCR instances that share the same context.
 * This implies that a Tag must be a plain-old-data type.
 *
 * \internal This also implies that a Tag must be some sort of shared union
 *           struct between backends.
 *
 * The size of the Tag shall always be a multiple of <tt>sizeof(int)</tt>.
 *
 * \internal This last requirement ensures we can have an array of tags and use
 *           memcpy to communicate just parts of it.
 */
class Tag
{
  private:
  /** A tag may not be default-constructed. */
  Tag() {}

  public:
  // there used to be a constructor here, but that precludes any backend from
  // managing a possibly constrained set of tags. Instead, we now use the same
  // mechanism as for #MemorySlot to have it tie to specific backends--
  // see MemorySpace::createTag

  /**
   * Releases all resources associated to this tag.
   *
   * \internal In OO programming, standard practice is to declare destructors
   *           virtual.
   */
  virtual ~Tag() {}

  /**
   * @returns a unique numerical identifier corresponding to this tag.
   *
   * The returned value is unique within the current HiCR instance. If a tag
   * is shared with other HiCR instances, there is a guarantee that each HiCR
   * instance returns the same ID.
   *
   * A call to this function on any valid instance of a Tag shall never
   * fail.
   *
   * \todo Do we really need this? We already specified that the Tag may be
   *       memcpy'd, so the Tag itself is already a unique identifier.
   *       Defining the return type to have some limited byte size also severely
   *       limits backends, and perhaps overly so.
   */
  HICR_API inline uint128_t getID() const noexcept;

  /**
   * @return The number of localities this tagSlot has been created with.
   *
   * This function never returns <tt>0</tt>. When given a valid Tag
   * instance, a call to this function never fails.
   *
   * When referring to localities corresponding to this tag, only IDs strictly
   * lower than the returned value are valid.
   */
  HICR_API inline size_t nLocalities() const noexcept;
};

} // end namespace HiCR
