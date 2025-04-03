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

namespace HiCR
{

// API annotation for inline functions, to make sure they are considered during coverage analysis
// This solution was taken from https://stackoverflow.com/a/10824832
#if defined __GNUC__ && defined ENABLE_COVERAGE
  #define __INLINE__ __attribute__((__used__)) inline
#else
  #define __INLINE__ inline
#endif

} // namespace HiCR
