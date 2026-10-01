// AUTO-DRAFT from redis/redis PR #15308
return NULL;
        }

        /* Consumer groups loading */
        uint64_t cgroups_count = rdbLoadLen(rdb,NULL);
        if (cgroups_count == RDB_LENERR) {
                cg_offset = streamEstimateDistanceFromFirstEverEntry(s,&cg_id);
            }

            streamCG *cgroup = streamCreateCG(s,cgname,sdslen(cgname),&cg_id,cg_offset);
            if (cgroup == NULL) {
                rdbReportCorruptRDB("Duplicated consumer group name %s",
