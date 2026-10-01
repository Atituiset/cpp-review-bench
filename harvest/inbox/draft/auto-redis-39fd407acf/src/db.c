// AUTO-DRAFT from redis/redis PR #14738
}
    }
  // <<< BUG ANCHOR
    if (server.io_threads_num > 1 && old->encoding == OBJ_ENCODING_RAW) {
        /* In multi-threaded mode, the OBJ_ENCODING_RAW string object usually is
         * allocated in the IO thread, so we defer the free to the IO thread.
         * Besides, we never free a string object in BIO threads, so, even with
