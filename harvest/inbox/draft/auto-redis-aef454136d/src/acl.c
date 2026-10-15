// AUTO-DRAFT from redis/redis PR #15803
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct {
    uint32_t flags; /* See SELECTOR_FLAG_* */
    /* The bit in allowed_commands is set if this user has the right to
     * execute this command.
     *
     * If the bit for a given command is NOT set and the command has
     * allowed first-args, Redis will also check allowed_firstargs in order to
     * understand if the command can be executed. */
    uint64_t allowed_commands[USER_COMMAND_BITS_COUNT/64];
    /* allowed_firstargs is used by ACL rules to block access to a command unless a
     * specific argv[1] is given.
     *
     * For each command ID (corresponding to the command bit set in allowed_commands),
     * This array points to an array of SDS strings, terminated by a NULL pointer,
     * with all the first-args that are allowed for this command. When no first-arg
     * matching is used, the field is just set to NULL to avoid allocating
     * USER_COMMAND_BITS_COUNT pointers. */
    sds **allowed_firstargs;
    list *patterns;  /* A list of allowed key patterns. If this field is NULL
                        the user cannot mention any key in a command, unless
                        the flag ALLKEYS is set in the user. */
    list *channels;  /* A list of allowed Pub/Sub channel patterns. If this
                        field is NULL the user cannot mention any channel in a
                        `PUBLISH` or [P][UNSUBSCRIBE] command, unless the flag
                        ALLCHANNELS is set in the user. */
    sds command_rules; /* A string representation of the ordered categories and commands, this
                        * is used to regenerate the original ACL string for display. */
} aclSelector;
/* …（同文件无关代码省略）… */
typedef struct {
    int flags; /* The ACL key permission types for this key pattern */
    sds pattern; /* The pattern to match keys against */
} keyPattern;
/* …（同文件无关代码省略）… */
int ACLGetCommandBitCoordinates(uint64_t id, uint64_t *word, uint64_t *bit) {
    if (id >= USER_COMMAND_BITS_COUNT) return C_ERR;
    *word = id / sizeof(uint64_t) / 8;
    *bit = 1ULL << (id % (sizeof(uint64_t) * 8));
    return C_OK;
}
/* …（同文件无关代码省略）… */
int ACLGetSelectorCommandBit(const aclSelector *selector, unsigned long id) {
    uint64_t word, bit;
    if (ACLGetCommandBitCoordinates(id,&word,&bit) == C_ERR) return 0;
    return (selector->allowed_commands[word] & bit) != 0;
}
/* …（同文件无关代码省略）… */
static int ACLSelectorCheckKey(aclSelector *selector, const char *key, int keylen, int keyspec_flags) {
    /* The selector can access any key */
    if (selector->flags & SELECTOR_FLAG_ALLKEYS) return ACL_OK;

    listIter li;
    listNode *ln;
    listRewind(selector->patterns,&li);

    int key_flags = 0;
    if (keyspec_flags & CMD_KEY_ACCESS) key_flags |= ACL_READ_PERMISSION;
    if (keyspec_flags & CMD_KEY_INSERT) key_flags |= ACL_WRITE_PERMISSION;
    if (keyspec_flags & CMD_KEY_DELETE) key_flags |= ACL_WRITE_PERMISSION;
    if (keyspec_flags & CMD_KEY_UPDATE) key_flags |= ACL_WRITE_PERMISSION;

    /* Is given key represent a prefix of a set of keys */
    int prefix = keyspec_flags & CMD_KEY_PREFIX;

    /* Test this key against every pattern. */
    while((ln = listNext(&li))) {
        keyPattern *pattern = listNodeValue(ln);
        if ((pattern->flags & key_flags) != key_flags)
            continue;
        size_t plen = sdslen(pattern->pattern);
        if (prefix) {
            if (prefixmatch(pattern->pattern,plen,key,keylen,0))
                return ACL_OK;
        } else {
            if (stringmatchlen(pattern->pattern, plen, key, keylen, 0))
                return ACL_OK;
        }
    }
    return ACL_DENIED_KEY;
}
/* …（同文件无关代码省略）… */
static int ACLCheckChannelAgainstList(list *reference, const char *channel, int channellen, int is_pattern) {
    listIter li;
    listNode *ln;

    listRewind(reference, &li);
    while((ln = listNext(&li))) {
        sds pattern = listNodeValue(ln);
        size_t plen = sdslen(pattern);
        /* Channel patterns are matched literally against the channels in
         * the list. Regular channels perform pattern matching. */
        if ((is_pattern && !strcmp(pattern,channel)) || 
            (!is_pattern && stringmatchlen(pattern,plen,channel,channellen,0)))
        {
            return ACL_OK;
        }
    }
    return ACL_DENIED_CHANNEL;
}
/* …（同文件无关代码省略）… */
typedef struct {
    int keys_init;
    getKeysResult keys;
} aclKeyResultCache;
/* …（同文件无关代码省略）… */
void initACLKeyResultCache(aclKeyResultCache *cache) {
    cache->keys_init = 0;
}
/* …（同文件无关代码省略）… */
void cleanupACLKeyResultCache(aclKeyResultCache *cache) {
    if (cache->keys_init) getKeysFreeResult(&(cache->keys));
}
/* …（同文件无关代码省略）… */
static int ACLSelectorCheckCmd(aclSelector *selector, struct redisCommand *cmd, robj **argv, int argc, int *keyidxptr, aclKeyResultCache *cache) {
    uint64_t id = cmd->id;
    int ret;
    if (!(selector->flags & SELECTOR_FLAG_ALLCOMMANDS) && !(cmd->flags & CMD_NO_AUTH)) {
        /* If the bit is not set we have to check further, in case the
         * command is allowed just with that specific first argument. */
        if (ACLGetSelectorCommandBit(selector,id) == 0) {
            /* Check if the first argument matches. */
            if (argc < 2 ||
                selector->allowed_firstargs == NULL ||
                selector->allowed_firstargs[id] == NULL)
            {
                return ACL_DENIED_CMD;
            }

            long subid = 0;
            while (1) {
                if (selector->allowed_firstargs[id][subid] == NULL)
                    return ACL_DENIED_CMD;
                int idx = cmd->parent ? 2 : 1;
                if (!strcasecmp(argv[idx]->ptr,selector->allowed_firstargs[id][subid]))
                    break; /* First argument match found. Stop here. */
                subid++;
            }
        }
    }

    /* Check if the user can execute commands explicitly touching the keys
     * mentioned in the command arguments. */
    if (!(selector->flags & SELECTOR_FLAG_ALLKEYS) && doesCommandHaveKeys(cmd)) {
        if (!(cache->keys_init)) {
            cache->keys = (getKeysResult) GETKEYS_RESULT_INIT;
            getKeysFromCommandWithSpecs(cmd, argv, argc, GET_KEYSPEC_DEFAULT, &(cache->keys));
            cache->keys_init = 1;
        }
        getKeysResult *result = &(cache->keys);
        keyReference *resultidx = result->keys;
        for (int j = 0; j < result->numkeys; j++) {
            int idx = resultidx[j].pos;
            ret = ACLSelectorCheckKey(selector, argv[idx]->ptr, sdslen(argv[idx]->ptr), resultidx[j].flags);
            if (ret != ACL_OK) {
                if (keyidxptr) *keyidxptr = resultidx[j].pos;
                return ret;
            }
        }
    }

    /* Check if the user can execute commands explicitly touching the channels
     * mentioned in the command arguments */
    const int channel_flags = CMD_CHANNEL_PUBLISH | CMD_CHANNEL_SUBSCRIBE;
    if (!(selector->flags & SELECTOR_FLAG_ALLCHANNELS) && doesCommandHaveChannelsWithFlags(cmd, channel_flags)) {
        getKeysResult channels = (getKeysResult) GETKEYS_RESULT_INIT;
        getChannelsFromCommand(cmd, argv, argc, &channels);
        keyReference *channelref = channels.keys;
        for (int j = 0; j < channels.numkeys; j++) {
            int idx = channelref[j].pos;
            if (!(channelref[j].flags & channel_flags)) continue;
            int is_pattern = channelref[j].flags & CMD_CHANNEL_PATTERN;
            int ret = ACLCheckChannelAgainstList(selector->channels, argv[idx]->ptr, sdslen(argv[idx]->ptr), is_pattern);
            if (ret != ACL_OK) {
                if (keyidxptr) *keyidxptr = channelref[j].pos;
                getKeysFreeResult(&channels);
                return ret;
            }
        }
        getKeysFreeResult(&channels);
    }
    return ACL_OK;
}
/* …（同文件无关代码省略）… */
int ACLCheckAllUserCommandPerm(user *u, struct redisCommand *cmd, robj **argv, int argc, getKeysResult *key_result, int *idxptr) {
    listIter li;
    listNode *ln;

    /* If there is no associated user, the connection can run anything. */
    if (u == NULL) return ACL_OK;

    /* Quick check if the user has all permissions, return early if so. */
    if (likely(listFirst(u->selectors) != NULL)) {
        aclSelector *s = listNodeValue(listFirst(u->selectors));
        const uint32_t all_perms = SELECTOR_FLAG_ALLCOMMANDS |
                                   SELECTOR_FLAG_ALLKEYS |
                                   SELECTOR_FLAG_ALLCHANNELS;
        if ((s->flags & all_perms) == all_perms) return ACL_OK;
    }

    /* We have to pick a single error to log, the logic for picking is as follows:
     * 1) If no selector can execute the command, return the command.
     * 2) Return the last key or channel that no selector could match. */
    int relevant_error = ACL_DENIED_CMD;
    int local_idxptr = 0, last_idx = 0;

    /* For multiple selectors, we cache the key result in between selector
     * calls to prevent duplicate lookups. */
    aclKeyResultCache cache;
    initACLKeyResultCache(&cache);
    if (key_result) {
        cache.keys = *key_result;
        cache.keys_init = 1;
    }

    /* Check each selector sequentially */
    listRewind(u->selectors,&li);
    while((ln = listNext(&li))) {
        aclSelector *s = (aclSelector *) listNodeValue(ln);
        int acl_retval = ACLSelectorCheckCmd(s, cmd, argv, argc, &local_idxptr, &cache);
        if (acl_retval == ACL_OK) {
            if (!key_result) cleanupACLKeyResultCache(&cache);
            return ACL_OK;
        }
        if (acl_retval > relevant_error ||
            (acl_retval == relevant_error && local_idxptr > last_idx))
        {
            relevant_error = acl_retval;
            last_idx = local_idxptr;
        }
    }

    *idxptr = last_idx;
    if (!key_result) cleanupACLKeyResultCache(&cache);
    return relevant_error;
}

/* High level API for checking if a client can execute the queued up command */
int ACLCheckAllPerm(client *c, int *idxptr) {
    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc, getClientCachedKeyResult(c), idxptr);
}

/* If 'new' can access all channels 'original' could then return NULL;
