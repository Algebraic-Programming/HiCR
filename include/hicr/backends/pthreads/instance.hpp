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

/**
 * @file instance.hpp
 * @brief This file implements the instance class for the Pthreads backend
 * @author L. Terracciano
 * @date 10/10/2025
 */

#pragma once

#include <pthread.h>
#include <memory>
#include <hicr/core/instance.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Implementation of the HiCR Instance
*/
class Instance : public HiCR::Instance
{
  public:

  /**
   * Constructor
   * 
   * \param[in] instanceId the id of the instance
   * \param[in] pthreadId the pthread id 
   * \param[in] rootInstanceId the id of root
  */
  Instance(const instanceId_t instanceId, const pthread_t pthreadId, const instanceId_t rootInstanceId)
    : HiCR::Instance(instanceId),
      _pthreadId(pthreadId),
      _rootInstanceId(rootInstanceId){};

  ~Instance() = default;

  bool isRootInstance() const override { return getId() == _rootInstanceId; };

  /**
   * Getter for pthread id
   * 
   * \return ptread id
  */
  pthread_t getPthreadId() const { return _pthreadId; }

  private:

  /**
   * Pthread id
  */
  const pthread_t _pthreadId;

  /**
   * Id of HiCR root instance
  */
  const instanceId_t _rootInstanceId;
};
} // namespace HiCR::backend::pthreads