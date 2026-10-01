// AUTO-DRAFT from redis/redis PR #14690
if (!TestAssertStringReply(ctx,RedisModule_CallReplyArrayElement(reply, 1),"1234",4)) goto fail;
  // <<< BUG ANCHOR
    T("foo", "E");
    if (!TestAssertErrorReply(ctx,reply,"ERR unknown command 'foo', with args beginning with: ",53)) goto fail;

    T("set", "Ec", "x");
    if (!TestAssertErrorReply(ctx,reply,"ERR wrong number of arguments for 'set' command",47)) goto fail;
