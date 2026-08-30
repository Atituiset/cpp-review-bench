// AUTO-DRAFT from redis/redis PR #15628
return DEFRAG_DONE;
}

static doneStatus defragStageHashTemplates(void *ctx, monotime endtime) {
    unsigned long *cursor = ctx;
    hashTemplateRegistry *reg = server.htemplates;
    if (reg == NULL || reg->by_id == NULL) return DEFRAG_DONE;

    unsigned long iterations = 0;
    while (*cursor < reg->by_id_next) {
        hashTemplate *tmpl = hashTemplateGetById(*cursor);
        (*cursor)++;
        if (tmpl == NULL) continue; /* freed or never-used slot */

        hashTemplateDefrag(tmpl);

        if (++iterations > 8) {
            iterations = 0;
            if (getMonotonicUs() >= endtime) return DEFRAG_NOT_DONE;
        }
    }
    return DEFRAG_DONE;
}

static void defragTmplRegistryCb(void *privdata, const dictEntry *de, dictEntryLink plink) {
    UNUSED(privdata); 
    UNUSED(de);
    UNUSED(plink);
}

/* Defrag a registry lookup dict. Keys/values (template/blob pointers) are
 * relocated by defragStageHashTemplates. */
static doneStatus defragRegistryDict(dict **dref, unsigned long *cursor, monotime endtime) {
    dictDefragFunctions fns = { .defragAlloc = activeDefragAlloc };
    unsigned long iterations = 0;
    do {
        *cursor = dictScanDefrag(*dref, *cursor, defragTmplRegistryCb, &fns, NULL);
        if (++iterations > 64) {
            iterations = 0;
            if (getMonotonicUs() >= endtime) return DEFRAG_NOT_DONE;
        }
    } while (*cursor != 0);
    dict *newd = dictDefragTables(*dref);

static doneStatus defragStageHashTemplatesByFields(void *ctx, monotime endtime) {
    if (server.htemplates == NULL) return DEFRAG_DONE;
    return defragRegistryDict(&server.htemplates->by_fields, ctx, endtime);
}

static doneStatus defragStageHashTemplatesByFieldsLp(void *ctx, monotime endtime) {
    if (server.htemplates == NULL) return DEFRAG_DONE;
    return defragRegistryDict(&server.htemplates->by_fields_lp, ctx, endtime);
}

static doneStatus defragStageHashTemplatesById(void *ctx, monotime endtime) {
    addDefragStage(defragLuaScripts, N
