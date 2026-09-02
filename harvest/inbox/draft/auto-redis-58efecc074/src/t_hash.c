// AUTO-DRAFT from redis/redis PR #15604
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define TMPL_CHUNK_SIZE 128  /* template ids per chunk */
/* …（同文件无关代码省略）… */
typedef struct tmplIdChunk {
    hashTemplate *slots[TMPL_CHUNK_SIZE]; /* id % TMPL_CHUNK -> template, NULL if free */
    unsigned int used;               /* used slots; chunk freed when 0 */
} tmplIdChunk;
/* …（同文件无关代码省略）… */
hashTemplate *hashTemplateGetById(uint64_t id) {
    size_t chunk_idx = id / TMPL_CHUNK_SIZE;
    if (chunk_idx >= htemplates->by_id_cap) return NULL;
    tmplIdChunk *chunk = htemplates->by_id[chunk_idx];
    return chunk ? chunk->slots[id % TMPL_CHUNK_SIZE] : NULL;
}
/* …（同文件无关代码省略）… */
void hashTemplateIncrKeyRef(hashTemplate *tmpl) {
    tmpl->key_refcount++;
    htemplates->total_key_refs++;
}
/* …（同文件无关代码省略）… */
static void hashTemplateFreeIfUnreferenced(hashTemplate *tmpl) {
    if (tmpl->key_refcount == 0 && tmpl->hold_refcount == 0)
        dictDelete(htemplates->by_fields, tmpl);
}
/* …（同文件无关代码省略）… */
static inline int canThreadWriteRegistry(void) {
    return pthread_equal(pthread_self(), server.main_thread_id) || moduleThreadHoldsGIL();
}
/* …（同文件无关代码省略）… */
static void hashTemplateDecrKeyRef(hashTemplate *tmpl) {
    serverAssert(canThreadWriteRegistry());
    serverAssert(tmpl);
    serverAssert(tmpl->key_refcount > 0);
    htemplates->total_key_refs--;
    if (--tmpl->key_refcount == 0) hashTemplateFreeIfUnreferenced(tmpl);
}
/* …（同文件无关代码省略）… */
static hashTemplate *hashTemplateArrayGetTemplate(hashTemplateArray *hta) {
    hashTemplate *tmpl = hashTemplateGetById(hta->tmpl_id);
    serverAssert(tmpl != NULL);
    return tmpl;
}

/* …（同文件无关代码省略）… */
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

/* …（同文件无关代码省略）… */
#define HASH_SET_TAKE_VALUE  (1<<1)
/* …（同文件无关代码省略）… */
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
/* …（同文件无关代码省略）… */
        } else {
            hashTemplateArray *hta = o->ptr;
            /* Expand struct and shift elements to make room. */
            hta = zrealloc(hta, sizeof(*hta) + sizeof(sds) * new_field_count);
            if ((unsigned long long)insert_pos < tmpl->field_count) {
                memmove(&hta->values[insert_pos + 1], &hta->values[insert_pos],
                        sizeof(sds) * (tmpl->field_count - insert_pos));
/* …（同文件无关代码省略）… */
            } else {
                hta->values[insert_pos] = sdsdup(value);
            }
            hashTemplateDecrKeyRef(tmpl);
            hta->tmpl_id = new_tmpl->id;
            hta->field_count = new_tmpl->field_count;
/* …（同文件无关代码省略）… */
                    o->ptr = lp;
                } else {
                    hashTemplateArray *hta = o->ptr;
                    sdsfree(hta->values[idx]);
                    memmove(&hta->values[idx], &hta->values[idx + 1],
                            sizeof(sds) * (old_count - idx - 1));
                    hta->tmpl_id = new_tmpl->id;
                    hta->field_count = new_count;
                    hta = zrealloc(hta, sizeof(*hta) + sizeof(sds) * new_count);
                    o->ptr = hta;
                }
                hashTemplateDecrKeyRef(tmpl);
/* …（同文件无关代码省略）… */
        size = lpBytes(lp);
    } else if (o->encoding == OBJ_ENCODING_TMPL_ARRAY) {
        hashTemplateArray *hta = o->ptr;
        size = sizeof(hashTemplateArray) + sizeof(sds) * hta->field_count;
        for (unsigned long long i = 0; i < hta->field_count; i++) {
            if (hta->values[i]) size += sdsAllocSize(hta->values[i]);
        }
    } else {
        serverPanic("Unknown hash encoding");
    }
/* …（同文件无关代码省略）… */
        hobj->encoding = OBJ_ENCODING_TMPL_LP;
    } else if (o->encoding == OBJ_ENCODING_TMPL_ARRAY) {
        hashTemplateArray *hta = o->ptr;
        unsigned long long n = hta->field_count;

        /* Create new array structure with duplicated values. */
        hashTemplateArray *new_hta = zmalloc(sizeof(*new_hta) + sizeof(sds) * n);
        new_hta->tmpl_id = hta->tmpl_id;
        new_hta->field_count = n;
        hashTemplateIncrKeyRef(hashTemplateGetById(new_hta->tmpl_id));
        for (unsigned long long i = 0; i < n; i++) {
            new_hta->values[i] = sdsdup(hta->values[i]);
        }

        hobj = createObject(OBJ_HASH, new_hta);
        hobj->encoding = OBJ_ENCODING_TMPL_ARRAY;
