// AUTO-DRAFT from redis/redis PR #15673
return relevant_error;
}

/* High level API for checking if a client can execute the queued up command */
int ACLCheckAllPerm(client *c, int *idxptr) {
    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc, getClientCachedKeyResult(c), idxptr);
}

/* If 'new' can access all channels 'original' could then return NULL;
