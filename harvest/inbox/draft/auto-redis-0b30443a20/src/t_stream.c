// AUTO-DRAFT from redis/redis PR #15489
}
  // <<< BUG ANCHOR
    s->last_id = id;
    if (entries_added != -1)
        s->entries_added = entries_added;
    if (!streamIDEqZero(&max_xdel_id))
        s->max_deleted_entry_id = max_xdel_id;
    addReply(c,shared.ok);
