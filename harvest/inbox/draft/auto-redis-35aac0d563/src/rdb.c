// AUTO-DRAFT from redis/redis PR #15171
/* search for duplicate records */
                        sds field = sdstrynewlen(fstr, flen);
                        if (!field || dictAdd(dupSearchDict, field, NULL) != DICT_OK ||  // <<< BUG ANCHOR
                            !ziplistSafeToAdd(zl, (size_t)flen + vlen)) {
                            rdbReportCorruptRDB("Hash zipmap with dup elements, or big length (%u)", flen);
                            dictRelease(dupSearchDict);
                            sdsfree(field);
                            zfree(encoded);
                            o->ptr = NULL;
                            decrRefCount(o);
                                                " loading a stream consumer "
                                                "group");
                        decrRefCount(o);
                        streamFreeNACK(nack);
                        return NULL;
                    }
                }
