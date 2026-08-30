// AUTO-DRAFT from redis/redis PR #15604
static hashTemplate *hashTemplateArrayGetTemplate(hashTemplateArray *hta) {
    hashTemplate *tmpl = hashTemplateGetById(hta->tmpl_id);
    serverAssert(tmpl != NULL);
    return tmpl;
}
  // <<< BUG ANCHOR
 * Otherwise copies them with sdsdup. */
static hashTemplateArray *hashTemplateArrayCreate(hashTemplate *tmpl, sds *values, int take) {
    unsigned long long n = tmpl->field_count;
    hashTemplateArray *hta = zmalloc(sizeof(*hta) + sizeof(sds) * n);
    hta->tmpl_id = tmpl->id;
    hta->field_count = n;

    for (unsigned long long i = 0; i < n; i++)
        hta->values[i] = take ? values[i] : sdsdup(values[i]);

    hashTemplateIncrKeyRef(tmpl);
    return hta;
}

/* Free a hashTemplateArray (release key ref and free data). May run in a BIO
 * lazyfree thread: uses the tmpl_id/field_count, never the registry. */
void hashTemplateArrayFree(hashTemplateArray *hta) {
    for (unsigned long long i = 0; i < hta->field_count; i++)
        sdsfree(hta->values[i]);

                serverAssert(o->ptr != NULL);
            } else {
                hashTemplateArray *hta = o->ptr;
                if (hta->values[field_idx]) sdsfree(hta->values[field_idx]);
                if (flags & HASH_SET_TAKE_VALUE) {
                    hta->values[field_idx] = value;  /* adopt, don't copy */
                    value = NULL;
                } else {
                    hta->values[field_idx] = sdsdup(value);
                }
            }
            update = 1;
            goto cleanup;
        } else {
            hashTemplateArray *hta = o->ptr;
            /* Expand struct and shift elements to make room. */
            hta = zrealloc(hta, sizeof(*hta) + sizeof(sds) * new_field_count);
            if ((unsigned long long)insert_pos < tmpl->field_count) {
                memmove(&hta->values[insert_pos + 1], &hta->values[insert_pos],
                        sizeof(sds) * (tmpl->field_count - insert_pos));
            } else {
                hta->values[insert_pos] = sdsdup(value);
           
