// AUTO-DRAFT from redis/redis PR #15659
reply = callReplyCreateError(err, ctx);
        goto cleanup;
    }
    if (!commandCheckArity(c, error_as_call_replies? &err : NULL)) {  // <<< BUG ANCHOR
        errno = EINVAL;
        if (error_as_call_replies)
            reply = callReplyCreateError(err, ctx);
 * REDISMODULE_ERR is returned and errno is set to the following values:
 *
 * * ENOENT: Specified command does not exist.
 * * EACCES: Command cannot be executed, according to ACL rules
 */
int RM_ACLCheckCommandPermissions(RedisModuleUser *user, RedisModuleString **argv, int argc) {
        return REDISMODULE_ERR;
    }

    if (ACLCheckAllUserCommandPerm(user->user, cmd, argv, argc, &keyidxptr) != ACL_OK) {
        errno = EACCES;
        return REDISMODULE_ERR;
