// AUTO-DRAFT from redis/redis PR #15508
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define debugnode(msg,n) raxDebugShowNode(msg,n)
/* …（同文件无关代码省略）… */
static inline void raxStackInit(raxStack *ts) {
    ts->stack = ts->static_items;
    ts->items = 0;
    ts->maxitems = RAX_STACK_STATIC_ITEMS;
    ts->oom = 0;
}
/* …（同文件无关代码省略）… */
static inline int raxStackPush(raxStack *ts, void *ptr) {
    if (ts->items == ts->maxitems) {
        if (ts->stack == ts->static_items) {
            ts->stack = rax_malloc(sizeof(void*)*ts->maxitems*2);
            if (ts->stack == NULL) {
                ts->stack = ts->static_items;
                ts->oom = 1;
                errno = ENOMEM;
                return 0;
            }
            memcpy(ts->stack,ts->static_items,sizeof(void*)*ts->maxitems);
        } else {
            void **newalloc = rax_realloc(ts->stack,sizeof(void*)*ts->maxitems*2);
            if (newalloc == NULL) {
                ts->oom = 1;
                errno = ENOMEM;
                return 0;
            }
            ts->stack = newalloc;
        }
        ts->maxitems *= 2;
    }
    ts->stack[ts->items] = ptr;
    ts->items++;
    return 1;
}
/* …（同文件无关代码省略）… */
static inline void *raxStackPop(raxStack *ts) {
    if (ts->items == 0) return NULL;
    ts->items--;
    return ts->stack[ts->items];
}
/* …（同文件无关代码省略）… */
static inline void raxStackFree(raxStack *ts) {
    if (ts->stack != ts->static_items) rax_free(ts->stack);
}
/* …（同文件无关代码省略）… */
#define raxPadding(nodesize) ((sizeof(void*)-(((nodesize)+4) % sizeof(void*))) & (sizeof(void*)-1))
/* …（同文件无关代码省略）… */
#define raxNodeFirstChildPtr(n) ((raxNode**) ( \
    (n)->data + \
    (n)->size + \
    raxPadding((n)->size)))
/* …（同文件无关代码省略）… */
#define raxNodeCurrentLength(n) ( \
    sizeof(raxNode)+(n)->size+ \
    raxPadding((n)->size)+ \
    ((n)->iscompr ? sizeof(raxNode*) : sizeof(raxNode*)*(n)->size)+ \
    (((n)->iskey && !(n)->isnull)*sizeof(void*)) \
)
/* …（同文件无关代码省略）… */
void raxFreeNode(rax *rax, raxNode *n) {
    size_t usable;
    rax_free_usable(n, &usable);
    if (rax->alloc_size) *rax->alloc_size -= usable;
}
/* …（同文件无关代码省略）… */
static inline int raxStepLenNode(const raxNode *n) {
    return n->iscompr ? (int)n->size : 1;
}
/* …（同文件无关代码省略）… */
void *raxGetData(raxNode *n) {
    if (n->isnull) return NULL;
    void **ndata =(void**)((char*)n+raxNodeCurrentLength(n)-sizeof(void*));
    void *data;
    memcpy(&data,ndata,sizeof(data));
    return data;
}
/* …（同文件无关代码省略）… */
static inline int raxSlotsAreValues(const rax *rax, size_t childDepth) {
    return rax->keyFixedLen && childDepth == (size_t)rax->keyFixedLen;
}
/* …（同文件无关代码省略）… */
static inline void raxFreeInvokeValueCb(void *data,
                                        void (*free_cb)(void *item),
                                        void (*free_cb_withctx)(void *item, void *ctx),
                                        void *ctx) {
    if (data == NULL) return;
    if (free_cb_withctx) free_cb_withctx(data, ctx);
    else if (free_cb) free_cb(data);
}
/* …（同文件无关代码省略）… */
static void raxFreeNodesWithCallback(rax *rax, raxNode *n,
                                     void (*free_cb)(void *item),
                                     void (*free_cb_withctx)(void *item, void *ctx),
                                     void *ctx)
{
    raxStack stack, depths;
    raxStackInit(&stack);
    raxStackInit(&depths);
    raxStackPush(&stack, n);
    raxStackPush(&depths, (void*)(uintptr_t)0);

    while (stack.items > 0) {
        raxNode *curr = raxStackPop(&stack);
        size_t depth = (size_t)(uintptr_t)raxStackPop(&depths);
        debugnode("free traversing", curr);

        int numchildren = curr->iscompr ? 1 : (int)curr->size;
        raxNode **cp = raxNodeFirstChildPtr(curr);

        size_t child_depth = depth + raxStepLenNode(curr);

        for (int i = 0; i < numchildren; i++) {
            void *slot;
            memcpy(&slot, cp + i, sizeof(slot));
            /* If fixed-length leaf inlining, slots hold values not raxNode*. */
            if (raxSlotsAreValues(rax, child_depth)) {
                raxFreeInvokeValueCb(slot, free_cb, free_cb_withctx, ctx);
            } else {
                raxStackPush(&stack, slot);
                raxStackPush(&depths, (void*)(uintptr_t)child_depth);
            }
        }

        debugnode("free depth-first", curr);
        if (curr->iskey && !curr->isnull)
            raxFreeInvokeValueCb(raxGetData(curr), free_cb, free_cb_withctx, ctx);
        raxFreeNode(rax, curr);
        rax->numnodes--;
    }

    raxStackFree(&depths);
    raxStackFree(&stack);
}
/* …（同文件无关代码省略）… */
void raxFreeWithCallback(rax *rax, void (*free_callback)(void*)) {
    raxFreeNodesWithCallback(rax, rax->head, free_callback, NULL, NULL);
    assert(rax->numnodes == 0);
    size_t *alloc_size = rax->alloc_size;
    size_t usable;
    rax_free_usable(rax, &usable);
    if (alloc_size) *alloc_size -= usable;
}
/* …（同文件无关代码省略）… */
void raxFree(rax *rax) {
    raxFreeWithCallback(rax,NULL);
}
/* …（同文件无关代码省略）… */
void raxIteratorDelChars(raxIterator *it, size_t count) {
    it->key_len -= count;
}
/* …（同文件无关代码省略）… */

    raxNode *n = it->node;
    while(steps > 0 || (!n->iskey && it->leaf_slot_idx < 0)) {
        int numchildren = n->iscompr ? 1 : n->size;
        int r = rand() % (numchildren+(n != it->rt->head));

        if (r == numchildren) {
            /* Go up: if parked on a virtual leaf, exit it (the leaf parent
             * stays as n). Otherwise pop the real parent. Either way, the
             * edge to strip is owned by the resulting `n`. */
            if (it->leaf_slot_idx >= 0)
                it->leaf_slot_idx = -1;
            else
                n = raxStackPop(&it->stack);
            int todel = n->iscompr ? n->size : 1;
            raxIteratorDelChars(it,todel);
        } else {
/* …（同文件无关代码省略）… */
void raxDebugShowNode(c
