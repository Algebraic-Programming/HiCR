/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Provides a definition for the memory space class.
 * @author A. N. Yzelman
 * @date 8/8/2023
 */

#pragma once

#include <hicr/channel.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * This is a specialisation of the Resource class in HiCR, meant to express a
 * hardware memory element within a hierarchy (E.g., Cache, RAM, HBM, Device)
 *
 * \todo There is currently no abstract Resource type that is not intended as
 *       a compute resource. Therefore `specialisation' in the above text
 *       seeems incorrect-- there is currently just two classes:
 *         -# Resource, which encapsulates a compute resource, and
 *         -# MemorySpace, which encapsulates a memory resource (and more
 *            specifically, and at current at least, a buffer memory).
 *
 * At present, HiCR will only deal with RAM and device memory. Each compute
 * resource has associated with it a MemorySpace in which it resides. In its
 * most broad representation, it corresponds to any single address space
 * exposed by a system.
 *
 * Memory spaces are exposed by backends. A memory space can return memory
 * slots that indicate some memory region within said space. Unlike memory
 * spaces, which are always exposed by a single backend, memory slots could
 * interact with multiple backends; for example, a host memory region could
 * serve as * source or destination for memory copies to and from accelerators
 * or other remote nodes, thus requiring interaction with, again e.g., an ACL
 * backend or an ibverbs backend.
 *
 * Memory slots can either be created via allocation or via registration of
 * (user) memory. In the former case, releasing the memory slot will deallocate
 * its memory, while in the latter case deallocation remains the responsibility
 * of the user.
 *
 * There is a many-to-one relation between compute resources and memory spaces.
 * HiCR thus exposes memory spaces via getter functions to any compute resource.
 *
 * In future, there should be two broad classes of MemorySpaces: caches and
 * buffers. The current API in essence assumes buffers; i.e., managed memory.
 *
 * The interface for caches do not allow for the derivation of memory slots.
 * Instead, they allow for abstract operations such as flush, prefetch, or
 * invalidate. Backends may or may not support such operations and may do so at
 * different degrees of accuracy -- depending also on the underlying hardware.
 */
class MemorySpace final
{
  private:

  const size_t _id;

  public:

  MemorySpace(const size_t id) : _id(id) {};

  /**
   * Returns the memory space ID
   *
   * \return Unique local identifier for the space
   */
  __USED__ inline size_t getID() const
  {
    return _id;
  }

};

} // end namespace HiCR
