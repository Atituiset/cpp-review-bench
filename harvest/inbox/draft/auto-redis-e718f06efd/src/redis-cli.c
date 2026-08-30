// AUTO-DRAFT from redis/redis PR #14863
fixed = -1;
                    if (reply) freeReplyObject(reply);
                    if (slot_nodes) listRelease(slot_nodes);
                    goto cleanup;
                }
                assert(reply->type == REDIS_REPLY_ARRAY);
