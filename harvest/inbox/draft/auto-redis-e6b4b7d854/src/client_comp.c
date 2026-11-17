// AUTO-DRAFT from redis/redis PR #16207
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct compressionType {
    int (*init_compress)(struct compressionState *st, int level);
    int (*init_decompress)(struct compressionState *st);
    int (*compress)(struct compressionState *st, int flush);
    int (*decompress)(struct compressionState *st);
    void (*end)(struct compressionState *st);
} compressionType;
/* …（同文件无关代码省略）… */
typedef struct {
    unsigned char *data;
    int size;
    int written;
    int consumed;
} tempBuf;
/* …（同文件无关代码省略）… */
struct compressionState {
    const compressionType *type;
    tempBuf input;  /* Buffer holding compressed data */
    tempBuf output; /* Buffer holding uncompressed(decompressed) data */
    union {
        ZSTD_CStream *zstdCCtx;  /* Zstd compression ctx */
        ZSTD_DStream *zstdDCtx;  /* Zstd decompression ctx */
    } ctx;
    int write_flush_pending;    /* write flush not yet completed */
    int frame_open;             /* Data has been fed to the compressor with
                                 * ZSTD_e_continue but the frame has not been
                                 * ended/flushed yet, so buffered data is still
                                 * waiting to be sent. */
    int read_flush_pending;    /* read flush not yet completed */
    mstime_t last_write;  /* Time since last write. Used to check if it's time
                           * to flush the buffer */
    compressionDirection dir;
    listNode *pending_data_node;  /* Node in the event loop's pending
                                   * decompression list (el->privdata[2]),
                                   * or NULL if not pending. */
    int handle_pending;  /* When set, clientConnRead only drains already-read
                          * compressed data without touching the socket. */
    size_t alloc_size;  /* Total allocated size of this struct plus the input/output
                         * buffers, captured at allocation time so memory accounting
                         * doesn't have to query the allocator on every call. */
};
/* …（同文件无关代码省略）… */
int decompressInto(compressionState *state, char *buf, size_t buflen) {
    if (buflen == 0)
      return 0;

    int consumed = 0;

    /* Decompress as much data as possible */
    while ((size_t)consumed < buflen &&
           (state->read_flush_pending ||
            state->input.written > state->input.consumed ||
            state->output.written > state->output.consumed))
    {
        /* Reset the decompressed buffer if all the available data is consumed */
        if (state->output.consumed == state->output.size) {
            state->output.written = 0;
            state->output.consumed = 0;
        }

        if ((state->read_flush_pending && state->output.size > state->output.written) ||
             state->input.written > state->input.consumed)
        {
            if (state->type->decompress(state) == -1) {
                return -1;
            }
        }

        /* Copy the decompressed data to the output buffer */
        if (state->output.written > state->output.consumed) {
            size_t nonconsumed_decompressed = state->output.written - state->output.consumed;
            int to_consume = min(buflen - consumed, nonconsumed_decompressed);
            memcpy(buf + consumed, state->output.data + state->output.consumed, to_consume);

            state->output.consumed += to_consume;
            consumed += to_consume;
        }
    }

    if (state->output.consumed == state->output.size)
    {
        state->output.written = 0;
        state->output.consumed = 0;
    }

    /* Reset the compressed buffer if we decompressed all the available data */
    if (state->input.consumed == state->input.size) {
        state->input.written = 0;
        state->input.consumed = 0;
    }

    serverAssert((size_t)consumed <= buflen);
    return consumed;
}

/* Read data from input_buf and decompress it immediately. The result is written
 * into output_buf.
 * Return number of bytes decompressed. *consumed stores number of bytes consumed
 * from input_buf.
 * Note, that we may have enough compressed data inside input buf so that decompressing
 * it will exceed output_len. The function must be ran in a loop until input_buf
 * is fully consumed - so make sure to have free space in output_buf on each call. */
int clientReadBufAndDecompress(client *c, char *input_buf, size_t input_len,
                             char *output_buf, size_t output_len, size_t *consumed)
{
/* …（同文件无关代码省略）… */

        int decompressed = decompressInto(state, output_buf + tot_decompressed,
                                          output_len - tot_decompressed);
        if (decompressed <= 0) {
            /* Compression library failure, we should close the connection */
            if (decompressed < 0)
                c->conn->state = CONN_STATE_CLOSED;
            break;
        }
        tot_decompressed += decompressed;
    }
