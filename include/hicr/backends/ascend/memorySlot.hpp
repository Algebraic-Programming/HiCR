/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the Ascend backend
 * @author S. M. Martin and L.Terracciano
 * @date 19/10/2023
 */
#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/common.hpp>
#include <hicr/memorySlot.hpp>
#include <deque>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * Represents a contiguous segment within a memory space, with a starting address, a size, and the Ascend device id
 */
class MemorySlot final : public HiCR::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the Ascend backend
   *
   * \param deviceId ascend device id this memory slot belongs
   * \param pointer if this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param size the size of the memory slot, assumed to be contiguous
   * \param dataBuffer the ACL data buffer created for the memory slot
   * \param globalTag for global memory slots, indicates the subset of global memory slots this belongs to
   * \param globalKey unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    deviceIdentifier_t deviceId,
    void *const pointer,
    size_t size,
    const aclDataBuffer *dataBuffer,
    const aclrtContext context,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey), _deviceId(deviceId), _dataBuffer(dataBuffer), _context(context){};

  /**
   * Default destructor
   */
  ~MemorySlot() = default;

  /**
   * Return the Ascend device id to which this memory slot belongs
   *
   * \return the Ascend device id to which this memory slot belongs
   */
  __USED__ inline const deviceIdentifier_t getDeviceId() const { return _deviceId; }

  /**
   * Return the ACL data buffer associated to the memory slot
   *
   * \return the ACL data buffer associated to the memory slot
   */
  __USED__ inline const aclDataBuffer *getDataBuffer() const { return _dataBuffer; }

  __USED__ inline const aclrtContext getContext() const { return _context; }

  /**
   * Add the \p stream to the list of the active ones that contains operations involving the memory slot
   *
   * \param stream the stream containing an operation on the memory slot
   */
  __USED__ inline void addActiveEvent(const aclrtStream stream, const aclrtEvent event)
  {
    if (!_activeStreams.contains(stream)) _activeStreams[stream] = std::deque<aclrtEvent>{};
    _activeStreams[stream].emplace_back(event);
  }

  __USED__ inline aclrtEvent popActiveEvent(const aclrtStream stream)
  {
    auto event = *_activeStreams.at(stream).begin();
    _activeStreams.at(stream).pop_front();
    return event;
  }

  __USED__ inline bool isInvolvedInStreams()
  {
    for (auto &[_, events] : _activeStreams)
    {
      if (!events.empty()) return true;
    }
    return false;
  }

  private:

  /**
   * The Ascend Device ID in which the memory slot is created
   */
  const deviceIdentifier_t _deviceId;

  /**
   * The Ascend Data Buffer associated with the memory slot
   */
  const aclDataBuffer *_dataBuffer;

  /**
   * Keep track of how many active streams operates on this memory slot
   */
  std::map<aclrtStream, std::deque<aclrtEvent>> _activeStreams;

  const aclrtContext _context;
};
} // namespace ascend
} // namespace backend
} // namespace HiCR
