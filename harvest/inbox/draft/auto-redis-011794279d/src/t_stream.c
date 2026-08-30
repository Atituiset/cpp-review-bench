// AUTO-DRAFT from redis/redis PR #15282
void streamFreeCGGeneric(void *cg, void *s);
void streamFreeNACK(stream *s, streamNACK *na);
size_t streamReplyWithRangeFromConsumerPEL(client *c, stream *s, streamID *start, streamID *end, size_t count, streamCG *group, streamConsumer *consumer);  // <<< BUG ANCHOR
int streamParseStrictIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq, int *seq_given);
int streamParseIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq);

    decrRefCount(argv[4]);
}

/* Send the stream items in the specified range to the client 'c'. The range
 * the client will receive is between start and end inclusive, if 'count' is
 * non zero, no more than 'count' elements are sent.
 * STREAM_RWR_CLAIMED: Return only claimable entries from the PEL. New entries
 *                     from the stream are not returned.
 *
 * The final argument 'spi' (stream propagation info pointer) is a structure
 * filled with information needed to propagate the command execution to AOF
 * and slaves, in the case a consumer group was passed: we need to generate
                                           boundaries, just the entries. */
#define STREAM_RWR_HISTORY (1<<2)       /* Only serve consumer local PEL. */
#define STREAM_RWR_CLAIMED (1<<3)       /* Only serve claimed entries from PEL. */
size_t streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, size_t count, int rev, long long min_idle_time, streamCG *group, streamConsumer *consumer, int flags, streamPropInfo *spi, unsigned long *propCount) {
    void *arraylen_ptr = NULL;
    size_t arraylen = 0;
    streamIterator si;
            uint64_t idle = cmd_time_snapshot - nack->delivery_time;
            if (idle < (uint64_t)min_idle_time) break;

            /* Process and claim this entry */
            uint64_t delivery_count = nack->delivery_count;

            decrRefCount(group_last_id);
        }
        return streamReplyWithRangeFromConsumerPEL(c,s,start,end,count,
                                                   group
