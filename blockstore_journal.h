#pragma once

#define MIN_JOURNAL_SIZE 4*1024*1024
#define JOURNAL_MAGIC 0x4A33

// Journal entries
// Journal entries are linked to each other by their crc32 value
// The journal is almost a blockchain, because object versions constantly increase
#define JE_START       0x01
#define JE_SMALL_WRITE 0x02
#define JE_BIG_WRITE   0x03
#define JE_STABLE      0x04
#define JE_DELETE      0x05

struct __attribute__((__packed__)) journal_entry_start
{
    uint16_t magic;
    uint16_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t reserved1;
    uint64_t offset;
};

struct __attribute__((__packed__)) journal_entry_small_write
{
    uint16_t magic;
    uint16_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t crc32_prev;
    object_id oid;
    uint64_t version;
    uint32_t offset;
    uint32_t len;
    // small_write entries contain <len> bytes of data, but data is stored in the next journal sector
};

struct __attribute__((__packed__)) journal_entry_big_write
{
    uint16_t magic;
    uint16_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t crc32_prev;
    object_id oid;
    uint64_t version;
    uint64_t block;
};

struct __attribute__((__packed__)) journal_entry_stable
{
    uint16_t magic;
    uint16_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t crc32_prev;
    object_id oid;
    uint64_t version;
};

struct __attribute__((__packed__)) journal_entry_del
{
    uint16_t magic;
    uint16_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t crc32_prev;
    object_id oid;
    uint64_t version;
};

struct __attribute__((__packed__)) journal_entry
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint16_t magic;
            uint16_t type;
            uint32_t size;
            uint32_t crc32;
        };
        journal_entry_start start;
        journal_entry_small_write small_write;
        journal_entry_big_write big_write;
        journal_entry_stable stable;
        journal_entry_del del;
    };
};
