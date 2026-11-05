// AUTO-DRAFT from redis/redis PR #15804
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
void resetClientQbufState(client *c) {
    c->reqtype = 0;
    c->multibulklen = 0;
    c->bulklen = -1;
}
/* …（同文件无关代码省略）… */
                pcmd->reploff = c->io_read_reploff - sdslen(c->querybuf) + c->qb_pos;

            preprocessCommand(c, pcmd);
            pcmd->flags |= PENDING_CMD_FLAG_PREPROCESSED;
            resetClientQbufState(c);
        }

/* …（同文件无关代码省略）… */
    return cmd;
}

/* Get cached key result for current pending command */
getKeysResult *getClientCachedKeyResult(client *c) {
    pendingCommand *pcmd = c->current_pending_cmd;
    if (pcmd) {
        /* Preprocess the command if needed */
        if (!(pcmd->flags & PENDING_CMD_FLAG_PREPROCESSED)) {
            preprocessCommand(c, pcmd);
            pcmd->flags |= PENDING_CMD_FLAG_PREPROCESSED;
        }

        /* Return cached result if available */
        if (pcmd->flags & PENDING_CMD_KEYS_RESULT_VALID)
            return &c->current_pending_cmd->keys_result;
    }
    return NULL;
}
