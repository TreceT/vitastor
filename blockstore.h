#pragma once

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdint.h>

#include <map>
#include <unordered_map>
#include <functional>

#include "ringloop.h"

// Default block size is 128 KB, current allowed range is 4K - 128M
#define DEFAULT_ORDER 17
#define MIN_BLOCK_SIZE 4*1024
#define MAX_BLOCK_SIZE 128*1024*1024
#define DISK_ALIGNMENT 512

// 16 bytes per object/stripe id
// stripe includes replica number in 4 least significant bits
struct __attribute__((__packed__)) object_id
{
    uint64_t inode;
    uint64_t stripe;
};

inline bool operator == (const object_id & a, const object_id & b)
{
    return a.inode == b.inode && a.stripe == b.stripe;
}

inline bool operator != (const object_id & a, const object_id & b)
{
    return a.inode != b.inode || a.stripe != b.stripe;
}

inline bool operator < (const object_id & a, const object_id & b)
{
    return a.inode < b.inode || a.inode == b.inode && a.stripe < b.stripe;
}

// 56 = 24 + 32 bytes per dirty entry in memory (obj_ver_id => dirty_entry)
struct __attribute__((__packed__)) obj_ver_id
{
    object_id oid;
    uint64_t version;
};

inline bool operator < (const obj_ver_id & a, const obj_ver_id & b)
{
    return a.oid < b.oid || a.oid == b.oid && a.version < b.version;
}

class oid_hash
{
public:
    size_t operator()(const object_id &s) const
    {
        size_t seed = 0;
        // Copy-pasted from spp::hash_combine()
        seed ^= (s.inode + 0xc6a4a7935bd1e995 + (seed << 6) + (seed >> 2));
        seed ^= (s.stripe + 0xc6a4a7935bd1e995 + (seed << 6) + (seed >> 2));
        return seed;
    }
};

#define OP_READ 1
#define OP_WRITE 2
#define OP_SYNC 3
#define OP_STABLE 4
#define OP_DELETE 5
#define OP_TYPE_MASK 0x7

#define BS_OP_PRIVATE_DATA_SIZE 256

struct blockstore_op_t
{
    // flags contain operation type and possibly other flags
    uint64_t flags;
    // finish callback
    std::function<void (blockstore_op_t*)> callback;
    // For reads, writes & deletes: oid is the requested object
    object_id oid;
    // For reads: version=0 -> last stable, version=UINT64_MAX -> last unstable, version=X -> specific version
    // For writes & deletes: a new version is assigned automatically
    uint64_t version;
    // For reads & writes: offset & len are the requested part of the object, buf is the buffer
    uint32_t offset;
    // For stabilize requests: buf contains <len> obj_ver_id's to stabilize
    uint32_t len;
    void *buf;
    int retval;

    uint8_t private_data[BS_OP_PRIVATE_DATA_SIZE];
};

typedef std::unordered_map<std::string, std::string> blockstore_config_t;

class blockstore_impl_t;

class blockstore_t
{
    blockstore_impl_t *impl;
public:
    blockstore_t(blockstore_config_t & config, ring_loop_t *ringloop);
    ~blockstore_t();

    // Event loop
    void loop();

    // Returns true when blockstore is ready to process operations
    // (Although you're free to enqueue them before that)
    bool is_started();

    // Returns true when it's safe to destroy the instance. If destroying the instance
    // requires to purge some queues, starts that process. Should be called in the event
    // loop until it returns true.
    bool is_safe_to_stop();

    // Submission
    void enqueue_op(blockstore_op_t *op);

    // Unstable writes are added here (map of object_id -> version)
    std::map<object_id, uint64_t> & get_unstable_writes();

    uint32_t get_block_size();
    uint32_t get_block_order();
    uint64_t get_block_count();
};
