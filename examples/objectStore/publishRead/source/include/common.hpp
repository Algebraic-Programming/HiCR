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

#define BLOCK_SIZE 4096 * 4096
#define OBJECT_STORE_TAG 42
#define CHANNEL_PAYLOAD_CAPACITY 1048576
#define CHANNEL_COUNT_CAPACITY 1024
#define CHANNEL_TAG 0xF000
#define SIZES_BUFFER_KEY 0
#define PRODUCER_COORDINATION_BUFFER_FOR_SIZES_KEY 1
#define PRODUCER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY 2
#define CONSUMER_COORDINATION_BUFFER_FOR_SIZES_KEY 3
#define CONSUMER_COORDINATION_BUFFER_FOR_PAYLOADS_KEY 4
#define CONSUMER_PAYLOAD_KEY 5