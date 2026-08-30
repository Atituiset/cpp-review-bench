// AUTO-DRAFT from redis/redis PR #15282
robj *groupname;
} streamPropInfo;
  // <<< BUG ANCHOR
/* Prototypes of exported APIs. */
struct client;

stream *streamNew(void);
void freeStream(stream *s);
unsigned long streamLength(const robj *subject);
size_t streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, size_t count, int rev, long long min_idle_time, streamCG *group, streamConsumer *consumer, int flags, streamPropInfo *spi, unsigned long *propCount);
void streamIteratorStart(streamIterator *si, stream *s, streamID *start, streamID *end, int rev);
int streamIteratorGetID(streamIterator *si, streamID *id, int64_t *numfields);
void streamIteratorGetField(streamIterator *si, unsigned char **fieldptr, unsigned char **valueptr, int64_t *fieldlen, int64_t *valuelen);
