/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file concurrent/sortedMap.hpp
 * @brief Provides generic support for parallel hash maps
 * @author S. M. Martin
 * @date 28/6/2024
 */

#pragma once

#include <map>
#include <mutex>
#include <hicr/core/definitions.hpp>

namespace HiCR::concurrent
{

/**
 * @brief Generic class type for concurrent queues
 *
 * Abstracts away the implementation of a concurrent map, providing thread-safety access.
 * This is a first naive implementation with an internal mutex, but it should be improved by using a lock-free implementation
 *
 * @tparam K Represents a key type 
 * @tparam V Represents a value type
 */
template <class K, class V, class C = std::greater<K>>
class SortedMap
{
  public:

  /**
   * Default constructor
  */
  SortedMap()  = default;
  ~SortedMap() = default;

  /**
   * Inserts a single element in the map
   *
   * \param[in] p The key/value pair to insert
   */
  __INLINE__ void insert(const std::pair<K, V> p)
  {
    _mutex.lock();
    _map.insert(p);
    _mutex.unlock();
  }

  /**
   * Tries to get a value, given the key passed
   *
   * \param[in] k The key to use to access the map
   * \return The value at the requested key. This will produce an exception if the key is not present
   */
  __INLINE__ V &at(const K k)
  {
    _mutex.lock();
    auto &v = _map.at(k);
    _mutex.unlock();
    return v;
  }

  /**
   * Erases a value from index
   *
   * \param[in] k The key to erase
   */
  __INLINE__ void erase(const K k)
  {
    _mutex.lock();
    _map.erase(k);
    _mutex.unlock();
  }

  /**
   * Implements a concurrency-safe overload for the operator []
   *
   * \param[in] k The key to erase
   * \return The value found; a default if not found.
   */
  __INLINE__ V &operator[](const K k)
  {
    _mutex.lock();
    auto &v = _map[k];
    _mutex.unlock();
    return v;
  }

  /**
   * Clears the map entirely
   */
  __INLINE__ void clear()
  {
    _mutex.lock();
    _map.clear();
    _mutex.unlock();
  }

  /**
   * Function to determine whether the map is currently empty or not
   *
   * The past tense in "was" is deliverate, since it can be proven the return value had that
   * value but it cannot be guaranteed that it still has it
   * 
   * \return True, if it is empty; false if its not. Possibly changed now.
   */
  __INLINE__ bool wasEmpty()
  {
    _mutex.lock();
    auto isEmpty = _map->empty();
    _mutex.unlock();
    return isEmpty;
  }

  /**
   * Function to determine the current possible size of the map
   *
   * The past tense in "was" is deliverate, since it can be proven the return value had that
   * value but it cannot be guaranteed that it still has it
   * 
   * \return The size of the map, as last read by this thread. Possibly changed now.
   */
  __INLINE__ size_t wasSize()
  {
    _mutex.lock();
    auto size = _map->size();
    _mutex.unlock();
    return size;
  }

  private:

  /*
  * Internal map storage
  */
  std::map<K, V, C> _map;

  /**
   * Storage for the internal mutex
   */
  std::mutex _mutex;
};

} // namespace HiCR::concurrent
