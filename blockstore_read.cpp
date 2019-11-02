#include "blockstore.h"

int blockstore::fulfill_read(blockstore_operation & read_op, uint32_t item_start, uint32_t item_end,
    uint32_t item_state, uint64_t item_version, uint64_t item_location)
{
    uint32_t cur_start = item_start;
    if (cur_start < read_op.offset + read_op.len && item_end > read_op.offset)
    {
        cur_start = cur_start < read_op.offset ? read_op.offset : cur_start;
        item_end = item_end > read_op.offset + read_op.len ? read_op.offset + read_op.len : item_end;
        auto fulfill_near = read_op.read_vec.lower_bound(cur_start);
        if (fulfill_near != read_op.read_vec.begin())
        {
            fulfill_near--;
            if (fulfill_near->first + fulfill_near->second.iov_len <= cur_start)
            {
                fulfill_near++;
            }
        }
        while (fulfill_near != read_op.read_vec.end() && fulfill_near->first < item_end)
        {
            if (fulfill_near->first > cur_start)
            {
                if (item_state == ST_IN_FLIGHT)
                {
                    // Pause until it's written somewhere
                    read_op.wait_for = WAIT_IN_FLIGHT;
                    read_op.wait_version = item_version;
                    return -1;
                }
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                if (!sqe)
                {
                    // Pause until there are more requests available
                    read_op.wait_for = WAIT_SQE;
                    return -1;
                }
                read_op.read_vec[cur_start] = (struct iovec){
                    read_op.buf + cur_start - read_op.offset,
                    fulfill_near->first - cur_start
                };
                io_uring_prep_readv(
                    sqe,
                    IS_JOURNAL(item_state) ? journal_fd : data_fd,
                    // FIXME: &read_op.read_vec is forbidden
                    &read_op.read_vec[cur_start], 1,
                    (IS_JOURNAL(item_state) ? journal_offset : data_offset) + item_location + cur_start - item_start
                );
                io_uring_sqe_set_data(sqe, 0/*read op link*/);
            }
            cur_start = fulfill_near->first + fulfill_near->second.iov_len;
            fulfill_near++;
        }
        if (cur_start < item_end)
        {
            if (item_state == ST_IN_FLIGHT)
            {
                // Pause until it's written somewhere
                read_op.wait_for = WAIT_IN_FLIGHT;
                read_op.wait_version = item_version;
                return -1;
            }
            struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
            if (!sqe)
            {
                // Pause until there are more requests available
                read_op.wait_for = WAIT_SQE;
                return -1;
            }
            read_op.read_vec[cur_start] = (struct iovec){
                read_op.buf + cur_start - read_op.offset,
                item_end - cur_start
            };
            io_uring_prep_readv(
                sqe,
                IS_JOURNAL(item_state) ? journal_fd : data_fd,
                &read_op.read_vec[cur_start], 1,
                (IS_JOURNAL(item_state) ? journal_offset : data_offset) + item_location + cur_start - item_start
            );
            io_uring_sqe_set_data(sqe, 0/*read op link*/);
        }
    }
    return 0;
}

int blockstore::read(blockstore_operation *read_op)
{
    auto clean_it = object_db.find(read_op->oid);
    auto dirty_it = dirty_queue.find(read_op->oid);
    if (clean_it == object_db.end() && dirty_it == object_db.end())
    {
        // region is not allocated - return zeroes
        memset(read_op->buf, 0, read_op->len);
        read_op->callback(read_op);
        return 0;
    }
    unsigned prev_sqe_pos = ring->sq.sqe_tail;
    uint64_t fulfilled = 0;
    if (dirty_it != object_db.end())
    {
        dirty_list dirty = dirty_it->second;
        for (int i = dirty.size()-1; i >= 0; i--)
        {
            if (read_op->flags == OP_READ_DIRTY || IS_STABLE(dirty[i].state))
            {
                if (fulfill_read(*read_op, dirty[i].offset, dirty[i].offset + dirty[i].size, dirty[i].state, dirty[i].version, dirty[i].location) < 0)
                {
                    // need to wait for something, undo added requests and requeue op
                    ring->sq.sqe_tail = prev_sqe_pos;
                    read_op->read_vec.clear();
                    submit_queue.push_front(read_op);
                    return 0;
                }
            }
        }
    }
    if (clean_it != object_db.end())
    {
        if (fulfill_read(*read_op, 0, block_size, ST_CURRENT, 0, clean_it->second.location) < 0)
        {
            // need to wait for something, undo added requests and requeue op
            ring->sq.sqe_tail = prev_sqe_pos;
            read_op->read_vec.clear();
            submit_queue.push_front(read_op);
            return 0;
        }
    }
    if (!read_op->read_vec.size())
    {
        // region is not allocated - return zeroes
        free(read_op);
        memset(read_op->buf, 0, read_op->len);
        read_op->callback(read_op);
        return 0;
    }
    int ret = io_uring_submit(ring);
    if (ret < 0)
    {
        throw new std::runtime_error(std::string("io_uring_submit: ") + strerror(-ret));
    }
    return 0;
}
