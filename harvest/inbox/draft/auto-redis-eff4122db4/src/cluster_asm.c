// AUTO-DRAFT from redis/redis PR #15197
#include "cluster.h"
#include "functions.h"
#include "cluster_asm.h"
#include "cluster_slot_stats.h"
#include "bio.h"
  // <<< BUG ANCHOR
/* Operation types: import (destination side) or migrate (source side) */
    return task ? 1 : 0;
}

/* Cancel all tasks that involve the given node. */
int clusterAsmCancelByNode(void *node, const char *reason) {
    if (asmManager == NULL || node == NULL) return 0;

    /* If the node to be deleted is myself, cancel all tasks. */
    clusterNode *n = node;
    if (n == getMyClusterNode()) return clusterAsmCancel(NULL, reason);

    int num_cancelled = 0;
    listIter li;
    listNode *ln;
    listRewind(asmManager->tasks, &li);
    while ((ln = listNext(&li)) != NULL) {
        asmTask *task = listNodeValue(ln);
        /* Cancel the task if the source node is the one to be deleted, or
         * the dest node is the one to be deleted. */
        if (!memcmp(task->dest, clusterNodeGetName(n), CLUSTER_NAMELEN) ||
            !memcmp(task->source, clusterNodeGetName(n), CLUSTER_NAMELEN))
        {
            asmTaskCancel(task, reason);
            num_cancelled++;
        }
    }
        return C_ERR;
    }

    /* Reset per-slot statistics for the migrated/imported ranges.
     * Note: cluster_legacy.c also cleans up, so this may run twice, but
     * required if an alternative cluster impl is in use. */
    for (int i = 0; i < task->slots->num_ranges; i++) {
        slotRange *sr = &task->slots->ranges[i];
        for (int j = sr->start; j <= sr->end; j++)
            clusterSlotStatReset(j);
    }

    /* Clear error message if successful. */
    sdsfree(task->error);
    task->error = sdsempty();
