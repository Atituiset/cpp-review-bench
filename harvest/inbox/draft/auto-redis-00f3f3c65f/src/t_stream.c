// AUTO-DRAFT from redis/redis PR #15569
int streamParseIDOrReply(client *c, robj *o, streamID *id, uint64_t missing_seq);
  // <<< BUG ANCHOR
int streamEntryIsReferenced(stream *s, streamID *id);
void streamCleanupEntryCGroupRefs(stream *s, streamID *id);
void streamUpdateCGroupLastId(stream *s, streamCG *cg, streamID *id);
void trackStreamClaimTimeouts(client *c, robj **keys, int numkeys, uint64_t expire_time);

    }
}

/* Remove all consumer group references to a specific stream message. */
void streamCleanupEntryCGroupRefs(stream *s, streamID *id) {
    if (!s->cgroups_ref) return;
    list *cglist;
    listIter li;
    listNode *ln;

    /* If message is not in any consumer group, nothing to do */
    if (!raxFind(s->cgroups_ref, buf, sizeof(streamID), (void **)&cglist))
        return;

    listRewind(cglist, &li);
    while ((ln = listNext(&li))) {

    raxRemove(s->cgroups_ref, buf, sizeof(streamID), NULL);
    listRelease(cglist);
}

/* Check if a stream entry is still referenced by any consumer group.
    stream *s = kv->ptr;
    size_t old_alloc = server.memory_tracking_enabled ? kvobjAllocSize(kv) : 0;
    int first_entry = 0;
    int deleted = 0;
    addReplyArrayLen(c, args.numids);
    for (int j = 0; j < args.numids; j++) {
        int res = XDELEX_NO_ID;
        streamID *id = &ids[j];
        unsigned char buf[sizeof(streamID)];
        streamEncodeID(buf,id);
            if (streamEntryIsReferenced(s, id))
                can_delete = 0;
        } else if (args.delete_strategy == DELETE_STRATEGY_DELREF) {
            streamCleanupEntryCGroupRefs(s, id);
        }

        if (can_delete) { /* can_delete being true doesn't guarantee the ID exists */
                    s->max_deleted_entry_id = *id;
                }
                deleted++;
                res = XDELEX_DELETED;
            } else {
                /* This id doesn't exist. */
            res = XDELEX_STILL_REFERENCED;
        }

        addReplyLongLong(c, res);
    }

    /* Update the stream's first ID. */
    if (deleted) {
        i
