// AUTO-DRAFT from redis/redis PR #15530
sender->shard_id,
                                    (unsigned long long)senderConfigEpoch,
                                    (unsigned long long)sender->configEpoch);
                        } else {
                            /* A failover occurred in the shard where `sender` belongs to and `sender` is no longer
                             * a primary. Update slot assignment to `master`, which is the new primary in the shard */
            }
        }

        /* Update our info about served slots.
         *
         * Note: this MUST happen after we update the master/slave state
