// Copyright (c) Vitaliy Filippov, 2019+
// License: VNPL-1.1 (see README.md for details)

#include <stdexcept>
#include "allocator.h"

#include <stdlib.h>
#include <malloc.h>

allocator::allocator(uint64_t blocks)
{
    if (blocks >= 0x80000000 || blocks <= 1)
    {
        throw std::invalid_argument("blocks");
    }
    uint64_t p2 = 1;
    total = 0;
    while (p2 * 64 < blocks)
    {
        total += p2;
        p2 = p2 * 64;
    }
    total += (blocks+63) / 64;
    mask = new uint64_t[total];
    size = free = blocks;
    last_one_mask = (blocks % 64) == 0
        ? UINT64_MAX
        : ((1l << (blocks % 64)) - 1);
    for (uint64_t i = 0; i < total; i++)
    {
        mask[i] = 0;
    }
}

allocator::~allocator()
{
    delete[] mask;
}

void allocator::set(uint64_t addr, bool value)
{
    if (addr >= size)
    {
        return;
    }
    uint64_t p2 = 1, offset = 0;
    while (p2 * 64 < size)
    {
        offset += p2;
        p2 = p2 * 64;
    }
    uint64_t cur_addr = addr;
    bool is_last = true;
    uint64_t value64 = value ? 1 : 0;
    while (1)
    {
        uint64_t last = offset + cur_addr/64;
        uint64_t bit = cur_addr % 64;
        if (((mask[last] >> bit) & 1) != value64)
        {
            if (is_last)
            {
                free += value ? -1 : 1;
            }
            if (value)
            {
                mask[last] = mask[last] | (1l << bit);
                if (mask[last] != (!is_last || cur_addr/64 < size/64
                    ? UINT64_MAX : last_one_mask))
                {
                    break;
                }
            }
            else
            {
                mask[last] = mask[last] & ~(1l << bit);
            }
            is_last = false;
            if (p2 > 1)
            {
                p2 = p2 / 64;
                offset -= p2;
                cur_addr /= 64;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

uint64_t allocator::find_free()
{
    uint64_t p2 = 1, offset = 0, addr = 0, f, i;
    while (p2 < size)
    {
        if (offset+addr >= total)
        {
            return UINT64_MAX;
        }
        uint64_t m = mask[offset + addr];
        for (i = 0, f = 1; i < 64; i++, f <<= 1)
        {
            if (!(m & f))
            {
                break;
            }
        }
        if (i == 64)
        {
            // No space
            return UINT64_MAX;
        }
        addr = (addr * 64) | i;
        offset += p2;
        p2 = p2 * 64;
    }
    return addr;
}

uint64_t allocator::get_free_count()
{
    return free;
}
