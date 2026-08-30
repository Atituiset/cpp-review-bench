// AUTO-DRAFT from redis/redis PR #14848
info->state = PREFETCH_DONE;
            continue;
        }
        info->ht_idx = HT_IDX_INVALID;
        info->current_entry = NULL;
        info->current_kv = NULL;
 * 3. Prefetch the keys and values for all commands in the current batch from
 *    the main dictionaries. */
void prefetchCommands(void) {
    if (!batch) return;  // <<< BUG ANCHOR

    /* Prefetch argv's for all pending commands */
    for (size_t i = 0; i < batch->pcmd_count; i++) {
