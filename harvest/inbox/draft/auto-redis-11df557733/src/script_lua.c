// AUTO-DRAFT from redis/redis PR #14847
if ((cmd = lookupCommand(argv, argc)) == NULL) {
        luaPushError(lua, "Invalid command passed to redis.acl_check_cmd()");
        raise_error = 1;
    } else {
        int keyidxptr;
        if (ACLCheckAllUserCommandPerm(rctx->original_client->user, cmd, argv, argc, NULL, &keyidxptr) != ACL_OK) {
