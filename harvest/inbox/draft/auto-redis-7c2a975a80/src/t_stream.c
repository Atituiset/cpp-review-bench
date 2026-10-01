// AUTO-DRAFT from redis/redis PR #14276
return 1;

    /* Check if the message is in any consumer group's PEL */
    unsigned char buf[sizeof(streamID)];
    streamEncodeID(buf, id);
    return raxFind(s->cgroups_ref, buf, sizeof(streamID), NULL);
