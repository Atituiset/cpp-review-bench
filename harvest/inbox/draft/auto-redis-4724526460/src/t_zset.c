// AUTO-DRAFT from redis/redis PR #16165
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define ZSL_OFFSET_NO_ELE   UINT16_MAX
/* …（同文件无关代码省略）… */
static inline zskiplistNodeInfo *zslGetNodeInfo(const zskiplistNode *node) {
    return (zskiplistNodeInfo *)&node->level[0].span;
}
/* …（同文件无关代码省略）… */
sds zslGetNodeElement(const zskiplistNode *node) {
    zskiplistNodeInfo *info = zslGetNodeInfo(node);
    debugServerAssert(info->sdsoffset != ZSL_OFFSET_NO_ELE);
    return (char*)node + info->sdsoffset;
}
/* …（同文件无关代码省略）… */
static double zzlStrtod(unsigned char *vstr, unsigned int vlen) {
    return fast_float_strtod((char*)vstr, vlen, NULL);
}
/* …（同文件无关代码省略）… */
static unsigned int zzlLength(unsigned char *zl) {
    return lpLength(zl)/2;
}
/* …（同文件无关代码省略）… */
unsigned long zsetLength(const robj *zobj) {
    unsigned long length = 0;
    if (zobj->encoding == OBJ_ENCODING_LISTPACK) {
        length = zzlLength(zobj->ptr);
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        length = ((const zset*)zobj->ptr)->zsl->length;
    } else if (zobj->encoding == OBJ_ENCODING_BTREE) {
        length = zbtreeLength(zobj->ptr);
    } else {
        serverPanic("Unknown sorted set encoding");
    }
    return length;
}
/* …（同文件无关代码省略）… */
void zsetTypeRandomElement(robj *zsetobj, unsigned long zsetsize, listpackEntry *key, double *score) {
    if (zsetobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs = zsetobj->ptr;
        dictEntry *de = dictGetFairRandomKey(zs->dict);
        zskiplistNode *znode = dictGetKey(de);
        sds s = zslGetNodeElement(znode);
        key->sval = (unsigned char*)s;
        key->slen = sdslen(s);
        if (score) {
            *score = znode->score;
        }
    } else if (zsetobj->encoding == OBJ_ENCODING_LISTPACK) {
        listpackEntry val;
        lpRandomPair(zsetobj->ptr, zsetsize, key, &val, 2);
        if (score) {
            if (val.sval) {
                *score = zzlStrtod(val.sval,val.slen);
            } else {
                *score = (double)val.lval;
            }
        }
    } else if (zsetobj->encoding == OBJ_ENCODING_BTREE) {
        zbtreeIterator iter;
        const unsigned char *ele;
        size_t len;
        unsigned long rank = randomULong() % zsetsize;
        serverAssert(zbtreeIteratorSeekRank(zsetobj->ptr, rank, &iter));
        serverAssert(zbtreeIteratorNext(&iter, 0, &ele, &len, score));
        key->sval = (unsigned char *)ele;
        key->slen = len;
    } else {
        serverPanic("Unknown zset encoding");
    }
}
/* …（同文件无关代码省略）… */
    if ((zobj = lookupKeyReadOrReply(c, key, reply)) == NULL || checkType(c, zobj, OBJ_ZSET)) {
        return;
    }
    if (server.memory_tracking_enabled)
        oldsize = kvobjAllocSize(zobj);
    serverAssertWithInfo(c, ele, sdsEncodedObject(ele));
/* …（同文件无关代码省略）… */
        uniq = 0;
    }

    /* If count is zero, serve it ASAP to avoid special cases later. */
    if (count == 0) {
        addReply(c,shared.emptyarray);
        return;
    }
/* …（同文件无关代码省略）… */
        return;
    }

    if (server.memory_tracking_enabled)
        oldsize = kvobjAllocSize(zset);
    zsetTypeRandomElement(zset, zsetLength(zset), &ele,NULL);
