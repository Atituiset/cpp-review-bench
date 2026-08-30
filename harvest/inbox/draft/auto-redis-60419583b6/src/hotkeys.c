// AUTO-DRAFT from redis/redis PR #14756
if (!hotkeys || !hotkeys->active) return;
    if (hotkeys->keys_result.numkeys == 0) return;
  // <<< BUG ANCHOR
    /* Don't update stats for nested calls */
    if (server.execution_nesting) return;

    serverAssert(hotkeys->current_client);
