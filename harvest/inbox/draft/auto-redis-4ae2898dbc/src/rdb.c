// AUTO-DRAFT from redis/redis PR #15203
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define rdbReportCorruptRDB(...) rdbReportError(1, __LINE__,__VA_ARGS__)
/* …（同文件无关代码省略）… */
#define isRestoreContext() \
    ((server.current_client == NULL || server.current_client->id == CLIENT_ID_AOF) ? 0 : 1)
/* …（同文件无关代码省略）… */
void rdbReportError(int corruption_error, int linenum, char *reason, ...) {
    va_list ap;
    char msg[1024];
    int len;

    len = snprintf(msg,sizeof(msg),
        "Internal error in RDB reading offset %llu, function at rdb.c:%d -> ",
        (unsigned long long)server.loading_loaded_bytes, linenum);
    va_start(ap,reason);
    vsnprintf(msg+len,sizeof(msg)-len,reason,ap);
    va_end(ap);

    if (isRestoreContext()) {
        /* If we're in the context of a RESTORE command, just propagate the error. */
        /* log in VERBOSE, and return (don't exit). */
        serverLog(LL_VERBOSE, "%s", msg);
        return;
    } else if (rdbCheckMode) {
        /* If we're inside the rdb checker, let it handle the error. */
        rdbCheckError("%s",msg);
    } else if (rdbFileBeingLoaded) {
        /* If we're loading an rdb file form disk, run rdb check (and exit) */
        serverLog(LL_WARNING, "%s", msg);
        char *argv[2] = {"",rdbFileBeingLoaded};
        if (anetIsFifo(argv[1])) {
            /* Cannot check RDB FIFO because we cannot reopen the FIFO and check already streamed data. */
            rdbCheckError("Cannot check RDB that is a FIFO: %s", argv[1]);
            return;
        }
        redis_check_rdb_main(2,argv,NULL);
    } else if (corruption_error) {
        /* In diskless loading, in case of corrupt file, log and exit. */
        serverLog(LL_WARNING, "%s. Failure loading rdb format", msg);
    } else {
        /* In diskless loading, in case of a short read (not a corrupt
         * file), log and proceed (don't exit). */
        serverLog(LL_WARNING, "%s. Failure loading rdb format from socket, assuming connection error, resuming operation.", msg);
        return;
    }
    serverLog(LL_WARNING, "Terminating server after rdb file reading failure.");
    exit(1);
}
/* …（同文件无关代码省略）… */

                        /* search for duplicate records */
                        sds field = sdstrynewlen(fstr, flen);
                        int field_added = (field != NULL && dictAdd(dupSearchDict, field, NULL) == DICT_OK);
                        if (!field_added || !lpSafeToAdd(lp, (size_t)flen + vlen)) {
                            rdbReportCorruptRDB("Hash zipmap with dup elements, or big length (%u)", flen);
                            /* If field was not added to dict, we still own it.
                             * If it was added, dict owns it and dictRelease will free it. */
                            if (!field_added) sdsfree(field);
                            dictRelease(dupSearchDict);
                            lpFree(lp);
                            zfree(encoded);
                            o->ptr = NULL;
/* …（同文件无关代码省略）… */
                        rdbReportCorruptRDB("Duplicated consumer PEL entry "
                                                " loading a stream consumer "
                                                "group");
                        decrRefCount(o);
                        return NULL;
                    }
                }
