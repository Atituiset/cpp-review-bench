// AUTO-DRAFT from redis/redis PR #14750
#include "cluster_slot_stats.h"
  // <<< BUG ANCHOR
#include <ctype.h>

/* -----------------------------------------------------------------------------
 * Key space handling
 * already "down" but it is fragile to rely on the update of the global state,
 * so we also handle it here.
 *
 * CLUSTER_REDIR_DOWN_STATE and CLUSTER_REDIR_DOWN_RO_STATE if the cluster is
 * down but the user attempts to execute a command that addresses one or more keys. */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot,
        return myself;
    }

    /* Base case: just return the right node. However, if this node is not
     * myself, set error_code to MOVED since we need to issue a redirection. */
    if (n != myself && error_code) *error_code = CLUSTER_REDIR_MOVED;
                                        "-%s %d %s:%d",
                                        (error_code == CLUSTER_REDIR_ASK) ? "ASK" : "MOVED",
                                        hashslot, clusterNodePreferredEndpoint(n), port));
    } else {
        serverPanic("getNodeByQuery() unknown error.");
    }
    slotRangeArrayFree(slots);
}

/* Slot range array iterator */
slotRangeArrayIter *slotRangeArrayGetIterator(slotRangeArray *slots) {
    slotRangeArrayIter *it = zmalloc(sizeof(*it));
 */
void sflushCommand(client *c) {
    int flags = EMPTYDB_NO_FLAGS, argc = c->argc;

    if (server.cluster_enabled == 0) {
        addReplyError(c,"This instance has cluster support disabled");
    slotRangeArray *slots = parseSlotRangesOrReply(c, argc, 1);
    if (!slots) return;

    /* Iterate and find the slot ranges that belong to this node. Save them in
     * a new slotRangeArray. It is allocated on heap since there is a chance
     * that FLUSH SYNC will be running as blocking ASYNC and only later reply
     * with slot ranges */
    unsigned char slots_to_flush[CLUSTER_SLOTS] = {0}; /* Requested slots to flush */
    slotRangeArray *myslots = NULL;
    for (int i = 0; i < slots->num_ra
