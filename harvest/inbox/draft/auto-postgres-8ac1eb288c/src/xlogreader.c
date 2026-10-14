// AUTO-DRAFT from postgres/postgres PR #2246c04e7b99519010cd47be636273a5700b0b0d
}

#ifdef USE_ZSTD
/* Release the zstd decomoression context. */
static void
XLogReaderFreeZstdContext(void *arg)
{
