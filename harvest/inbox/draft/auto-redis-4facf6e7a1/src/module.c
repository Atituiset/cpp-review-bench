// AUTO-DRAFT from redis/redis PR #15371
moduleNotifyUserChanged(ctx->client);
  // <<< BUG ANCHOR
    ctx->client->user = user;
    ctx->client->authenticated = 1;

    if (clientHasModuleAuthInProgress(ctx->client)) {
        ctx->client->flags |= CLIENT_MODULE_AUTH_HAS_RESULT;
