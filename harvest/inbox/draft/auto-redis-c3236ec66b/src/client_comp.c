// AUTO-DRAFT from redis/redis PR #15499
* or NULL if not pending. */
    int handle_pending;  /* When set, clientConnRead only drains already-read
                          * compressed data without touching the socket. */
};
  // <<< BUG ANCHOR
/* --- zstd --- */

    /* temp buf storing compressed data */
    size_t outSize = ZSTD_CStreamOutSize();
    st->output.data = zmalloc(outSize);
    st->output.size = outSize;
    st->output.written = 0;
    st->output.consumed = 0;

    /* temp buf storing uncompressed data */
    size_t inSize = ZSTD_CStreamInSize();
    st->input.data = zmalloc(inSize);
    st->input.size = inSize;
    st->input.written = 0;
    st->input.consumed = 0;

    st->write_flush_pending = 0;


    /* temp buf storing compressed data */
    size_t inSize = ZSTD_DStreamInSize();
    st->input.data = zmalloc(inSize);
    st->input.size = inSize;
    st->input.written = 0;
    st->input.consumed = 0;

    /* temp buf storing decompressed data */
    size_t outSize = ZSTD_DStreamOutSize();
    st->output.data = zmalloc(outSize);
    st->output.size = outSize;
    st->output.written = 0;
    st->output.consumed = 0;

    st->read_flush_pending = 0;


/* Create compression state for the client */
int compressionStateCreate(client *c) {
    compressionState *st = zcalloc(sizeof(compressionState));
    st->type = &zstdType;
    st->last_write = 0;
    st->write_flush_pending = 0;
           state->output.written > state->output.consumed;
}

/* Add the client to its event loop's pending decompression list so its buffered
 * compressed/decompressed data can be drained from beforeSleep even when no
 * socket read event fires. No-op if already present. */
    return 0;
}

int clientConnWrite(client *c, const void *data, size_t len, int *nwritten) {
    int w = connWrite(c->conn, data, len);
    if (nwritten) *nwritten = (w > 0) ? w : 0;
