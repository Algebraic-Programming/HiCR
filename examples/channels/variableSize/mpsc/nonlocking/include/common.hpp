/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>
#include <vector>
#include <iterator>

#define SIZES_BUFFER_KEY 0
#define CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY 1
#define CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY 2
#define PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY 3
#define PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY 4
#define CONSUMER_PAYLOAD_KEY 5
#define CONSUMER_TOKEN_KEY 6
#define MESSAGES_PER_PRODUCER 5
#define ELEMENT_TYPE unsigned int

using namespace std;
template <typename T>
class Printer
{
  public:

  static void printBytes(const string prepend, void *buffer, size_t channelCapacity, size_t startIndex, size_t bytes)
  {
    if (bytes > channelCapacity)
    {
      std::cout << "Bytes larger than channel capacity, will not print\n";
      return;
    }

    cout << "=====\n";
    cout << prepend << " reading " << bytes << " bytes ";
    if (buffer == NULL)
    {
      cerr << "buffer is NULL in print routine" << endl;
      abort();
    }
    if (startIndex + bytes <= channelCapacity)
    {
      vector v(static_cast<T *>(buffer) + startIndex / sizeof(T), static_cast<T *>(buffer) + (startIndex + bytes) / sizeof(ELEMENT_TYPE));
      copy(v.begin(), v.end(), ostream_iterator<T>(cout, ","));
    }
    else
    {
      vector v1(static_cast<T *>(buffer) + startIndex / sizeof(T), static_cast<T *>(buffer) + channelCapacity / sizeof(ELEMENT_TYPE));
      vector v2(static_cast<T *>(buffer), static_cast<T *>(buffer) + (bytes + startIndex - channelCapacity) / sizeof(ELEMENT_TYPE));
      copy(v1.begin(), v1.end(), ostream_iterator<T>(cout, ","));
      copy(v2.begin(), v2.end(), ostream_iterator<T>(cout, ","));
    }
    cout << "\n=====\n";
  }
};
