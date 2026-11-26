// AUTO-DRAFT from redis/redis PR #16371
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
sds kvobjGetKey(const kvobj *kv) {
    static const uint8_t kvobjKeyHdrSize[4] = {
        sizeof(struct sdshdr5), sizeof(struct sdshdr8),
        sizeof(struct sdshdr16), sizeof(struct sdshdr32)
    };
    debugServerAssert(kv->iskvobj);
    kvBits *bits = kvobjBits(kv);
    return (sds) ((char *) (bits + 1) + kvobjKeyHdrSize[bits->key_sds_type]);
}
/* …（同文件无关代码省略）… */
size_t stringObjectAllocSize(const robj *o) {
    serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
    if(o->encoding == OBJ_ENCODING_INT) {
        /* Value already counted (reuse the "ptr" in header to store int) */
        return 0;
    } else if(o->encoding == OBJ_ENCODING_RAW) {
        return sdsAllocSize(o->ptr);
    } else if(o->encoding == OBJ_ENCODING_EMBSTR) {
        /* Value already counted (Value embedded in the object as well) */
        return 0;
    } else {
        serverPanic("Unknown string encoding");
    }
}
/* …（同文件无关代码省略）… */
    {
        return kvobjAllocSize(o);
    } else if (o->type == OBJ_MODULE) {
        return zmalloc_size(o) + moduleGetMemUsage(key, o, sample_size, dbid);
    }
    serverPanic("Unknown object type");
}
/* …（同文件无关代码省略）… */
size_t kvobjAllocSize(kvobj *o) {
    debugServerAssert(o->iskvobj);
    size_t asize = sizeof(kvobj) + sizeof(kvBits);
    /* Add metadata size */
    asize += getNumMeta(o->metabits) * sizeof(uint64_t);
    /* Add embedded key size */
    asize += sdsAllocSize(kvobjGetKey(o));
    /* Add embedded string size */
    if (o->encoding == OBJ_ENCODING_EMBSTR)
        asize += sdsAllocSize(o->ptr);

    if (o->type == OBJ_STRING) {
        asize += stringObjectAllocSize(o);
    } else if (o->type == OBJ_LIST) {
        asize += listTypeAllocSize(o);
    } else if (o->type == OBJ_SET) {
        asize += setTypeAllocSize(o);
    } else if (o->type == OBJ_ZSET) {
        asize += zsetAllocSize(o);
    } else if (o->type == OBJ_HASH) {
        asize += hashTypeAllocSize(o);
    } else if (o->type == OBJ_STREAM) {
        stream *s = o->ptr;
        asize += s->alloc_size;
#ifdef ENABLE_GCRA
    } else if (o->type == OBJ_GCRA) {
        asize += gcraTypeAllocSize(o);
#endif
    } else if (o->type == OBJ_ARRAY) {
        redisArray *ar = o->ptr;
        asize += ar->alloc_size;
    } else if (o->type == OBJ_BITMAP) {
        asize += bitroarAllocSize(o);
    } else if (o->type == OBJ_MODULE) {
        /* TODO: Provide moduleGetAllocSize() module API for O(1) allocation size retrieval */
    }
    return asize;
}
/* …（同文件无关代码省略）… */
size_t gcraTypeAllocSize(robj *o) {
    (void)o;
#if UINTPTR_MAX == 0xffffffff
    return sizeof(long long);
#else
    /* Same as string with int encoding there is no allocation as the value is
     * cast to void* and stored in o->ptr */
    return 0;
#endif
}
