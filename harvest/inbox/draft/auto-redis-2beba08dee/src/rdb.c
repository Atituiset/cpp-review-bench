// AUTO-DRAFT from redis/redis PR #15539
if (fields_lp) {
                    /* Get listpack blob and skip caching in fork. */
                    int cache = (server.in_fork_child == CHILD_TYPE_NONE);
                    unsigned char *blob = hashTemplateGetFieldsLp(tmpl, cache);
                    n = rdbSaveRawString(rdb, blob, lpBytes(blob));
                    if (!cache) lpFree(blob);
                    if (n == -1) return -1;
    rdbFreeSdsArray(out->fields, out->field_count);
    out->fields = NULL;
    if (out->fields_lp != NULL) {
        hashTemplateIndexFieldsLp(tmpl, out->fields_lp); /* transfers ownership */
        out->fields_lp = NULL;
    }
    return tmpl;
