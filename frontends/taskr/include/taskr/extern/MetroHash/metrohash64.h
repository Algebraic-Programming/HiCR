// metrohash64.h
//
// Copyright 2015-2018 J. Andrew Rogers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef METROHASH_METROHASH_64_H
#define METROHASH_METROHASH_64_H

#include <stdint.h>
#include <cstring>
#include "platform.h"

class MetroHash64
{
public:
    static const uint32_t bits = 64;

    // Constructor initializes the same as Initialize()
    MetroHash64(const uint64_t seed=0)
    {
        Initialize(seed);
    }
    
    // Initializes internal state for new hash with optional seed
    void Initialize(const uint64_t seed=0)
    {
        vseed = (static_cast<uint64_t>(seed) + k2) * k0;

        // initialize internal hash registers
        state.v[0] = vseed;
        state.v[1] = vseed;
        state.v[2] = vseed;
        state.v[3] = vseed;

        // initialize total length of input
        bytes = 0;
    }
    
    // Update the hash state with a string of bytes. If the length
    // is sufficiently long, the implementation switches to a bulk
    // hashing algorithm directly on the argument buffer for speed.
    void Update(const uint8_t * buffer, const uint64_t length)
    {
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(buffer);
        const uint8_t * const end = ptr + length;

        // input buffer may be partially filled
        if (bytes % 32)
        {
            uint64_t fill = 32 - (bytes % 32);
            if (fill > length)
                fill = length;

            memcpy(input.b + (bytes % 32), ptr, static_cast<size_t>(fill));
            ptr   += fill;
            bytes += fill;

            // input buffer is still partially filled
            if ((bytes % 32) != 0) return;

            // process full input buffer
            state.v[0] += read_u64(&input.b[ 0]) * k0; state.v[0] = rotate_right(state.v[0],29) + state.v[2];
            state.v[1] += read_u64(&input.b[ 8]) * k1; state.v[1] = rotate_right(state.v[1],29) + state.v[3];
            state.v[2] += read_u64(&input.b[16]) * k2; state.v[2] = rotate_right(state.v[2],29) + state.v[0];
            state.v[3] += read_u64(&input.b[24]) * k3; state.v[3] = rotate_right(state.v[3],29) + state.v[1];
        }

        // bulk update
        bytes += static_cast<uint64_t>(end - ptr);
        while (ptr <= (end - 32))
        {
            // process directly from the source, bypassing the input buffer
            state.v[0] += read_u64(ptr) * k0; ptr += 8; state.v[0] = rotate_right(state.v[0],29) + state.v[2];
            state.v[1] += read_u64(ptr) * k1; ptr += 8; state.v[1] = rotate_right(state.v[1],29) + state.v[3];
            state.v[2] += read_u64(ptr) * k2; ptr += 8; state.v[2] = rotate_right(state.v[2],29) + state.v[0];
            state.v[3] += read_u64(ptr) * k3; ptr += 8; state.v[3] = rotate_right(state.v[3],29) + state.v[1];
        }

        // store remaining bytes in input buffer
        if (ptr < end)
            memcpy(input.b, ptr, static_cast<size_t>(end - ptr));
    }

    // Constructs the final hash and writes it to the argument buffer.
    // After a hash is finalized, this instance must be Initialized()-ed
    // again or the behavior of Update() and Finalize() is undefined.
    void Finalize(uint8_t * const hash)
    {
        // finalize bulk loop, if used
        if (bytes >= 32)
        {
            state.v[2] ^= rotate_right(((state.v[0] + state.v[3]) * k0) + state.v[1], 37) * k1;
            state.v[3] ^= rotate_right(((state.v[1] + state.v[2]) * k1) + state.v[0], 37) * k0;
            state.v[0] ^= rotate_right(((state.v[0] + state.v[2]) * k0) + state.v[3], 37) * k1;
            state.v[1] ^= rotate_right(((state.v[1] + state.v[3]) * k1) + state.v[2], 37) * k0;

            state.v[0] = vseed + (state.v[0] ^ state.v[1]);
        }

        // process any bytes remaining in the input buffer
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(input.b);
        const uint8_t * const end = ptr + (bytes % 32);

        if ((end - ptr) >= 16)
        {
            state.v[1]  = state.v[0] + (read_u64(ptr) * k2); ptr += 8; state.v[1] = rotate_right(state.v[1],29) * k3;
            state.v[2]  = state.v[0] + (read_u64(ptr) * k2); ptr += 8; state.v[2] = rotate_right(state.v[2],29) * k3;
            state.v[1] ^= rotate_right(state.v[1] * k0, 21) + state.v[2];
            state.v[2] ^= rotate_right(state.v[2] * k3, 21) + state.v[1];
            state.v[0] += state.v[2];
        }

        if ((end - ptr) >= 8)
        {
            state.v[0] += read_u64(ptr) * k3; ptr += 8;
            state.v[0] ^= rotate_right(state.v[0], 55) * k1;
        }

        if ((end - ptr) >= 4)
        {
            state.v[0] += read_u32(ptr) * k3; ptr += 4;
            state.v[0] ^= rotate_right(state.v[0], 26) * k1;
        }

        if ((end - ptr) >= 2)
        {
            state.v[0] += read_u16(ptr) * k3; ptr += 2;
            state.v[0] ^= rotate_right(state.v[0], 48) * k1;
        }

        if ((end - ptr) >= 1)
        {
            state.v[0] += read_u8 (ptr) * k3;
            state.v[0] ^= rotate_right(state.v[0], 37) * k1;
        }

        state.v[0] ^= rotate_right(state.v[0], 28);
        state.v[0] *= k0;
        state.v[0] ^= rotate_right(state.v[0], 29);

        bytes = 0;

        // do any endian conversion here

        memcpy(hash, state.v, 8);
    }

    // A non-incremental function implementation. This can be significantly
    // faster than the incremental implementation for some usage patterns.
    static void Hash(const uint8_t * buffer, const uint64_t length, uint8_t * const hash, const uint64_t seed=0)
    {
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(buffer);
        const uint8_t * const end = ptr + length;

        uint64_t h = (static_cast<uint64_t>(seed) + k2) * k0;

        if (length >= 32)
        {
            uint64_t v[4];
            v[0] = h;
            v[1] = h;
            v[2] = h;
            v[3] = h;

            do
            {
                v[0] += read_u64(ptr) * k0; ptr += 8; v[0] = rotate_right(v[0],29) + v[2];
                v[1] += read_u64(ptr) * k1; ptr += 8; v[1] = rotate_right(v[1],29) + v[3];
                v[2] += read_u64(ptr) * k2; ptr += 8; v[2] = rotate_right(v[2],29) + v[0];
                v[3] += read_u64(ptr) * k3; ptr += 8; v[3] = rotate_right(v[3],29) + v[1];
            }
            while (ptr <= (end - 32));

            v[2] ^= rotate_right(((v[0] + v[3]) * k0) + v[1], 37) * k1;
            v[3] ^= rotate_right(((v[1] + v[2]) * k1) + v[0], 37) * k0;
            v[0] ^= rotate_right(((v[0] + v[2]) * k0) + v[3], 37) * k1;
            v[1] ^= rotate_right(((v[1] + v[3]) * k1) + v[2], 37) * k0;
            h += v[0] ^ v[1];
        }

        if ((end - ptr) >= 16)
        {
            uint64_t v0 = h + (read_u64(ptr) * k2); ptr += 8; v0 = rotate_right(v0,29) * k3;
            uint64_t v1 = h + (read_u64(ptr) * k2); ptr += 8; v1 = rotate_right(v1,29) * k3;
            v0 ^= rotate_right(v0 * k0, 21) + v1;
            v1 ^= rotate_right(v1 * k3, 21) + v0;
            h += v1;
        }

        if ((end - ptr) >= 8)
        {
            h += read_u64(ptr) * k3; ptr += 8;
            h ^= rotate_right(h, 55) * k1;
        }

        if ((end - ptr) >= 4)
        {
            h += read_u32(ptr) * k3; ptr += 4;
            h ^= rotate_right(h, 26) * k1;
        }

        if ((end - ptr) >= 2)
        {
            h += read_u16(ptr) * k3; ptr += 2;
            h ^= rotate_right(h, 48) * k1;
        }

        if ((end - ptr) >= 1)
        {
            h += read_u8 (ptr) * k3;
            h ^= rotate_right(h, 37) * k1;
        }

        h ^= rotate_right(h, 28);
        h *= k0;
        h ^= rotate_right(h, 29);

        memcpy(hash, &h, 8);
    }

private:
    static const uint64_t k0 = 0xD6D018F5;
    static const uint64_t k1 = 0xA2AA033B;
    static const uint64_t k2 = 0x62992FC1;
    static const uint64_t k3 = 0x30BC5B29;

    struct { uint64_t v[4]; } state;
    struct { uint8_t b[32]; } input;
    uint64_t bytes;
    uint64_t vseed;
};


#endif // #ifndef METROHASH_METROHASH_64_H

