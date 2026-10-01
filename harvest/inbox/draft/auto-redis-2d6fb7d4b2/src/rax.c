// AUTO-DRAFT from redis/redis PR #15103
return 1;
}
  // <<< BUG ANCHOR
/* This is the core of raxFree(): performs a depth-first scan of the
 * tree and releases all the nodes found. */
void raxRecursiveFree(rax *rax, raxNode *n, void (*free_callback)(void*)) {
    debugnode("free traversing",n);
    int numchildren = n->iscompr ? 1 : n->size;
    raxNode **cp = raxNodeLastChildPtr(n);
    while(numchildren--) {
        raxNode *child;
        memcpy(&child,cp,sizeof(child));
        raxRecursiveFree(rax,child,free_callback);
        cp--;
    }
    debugnode("free depth-first",n);
    if (free_callback && n->iskey && !n->isnull)
        free_callback(raxGetData(n));
    raxFreeNode(rax,n);
    rax->numnodes--;
}

/* Same as raxRecursiveFree() with context argument */
void raxRecursiveFreeWithCtx(rax *rax, raxNode *n,
                            void (*free_callback)(void *item, void *ctx), void *ctx) {
    debugnode("free traversing",n);
    int numchildren = n->iscompr ? 1 : n->size;
    raxNode **cp = raxNodeLastChildPtr(n);
    while(numchildren--) {
        raxNode *child;
        memcpy(&child,cp,sizeof(child));
        raxRecursiveFreeWithCtx(rax,child,free_callback, ctx);
        cp--;
    }
    debugnode("free depth-first",n);
    if (free_callback && n->iskey && !n->isnull)
        free_callback(raxGetData(n), ctx);
    raxFreeNode(rax,n);
    rax->numnodes--;
}

/* Free a whole radix tree, calling the specified callback in order to
 * free the auxiliary data. */
void raxFreeWithCallback(rax *rax, void (*free_callback)(void*)) {
    raxRecursiveFree(rax,rax->head,free_callback);
    assert(rax->numnodes == 0);
    size_t *alloc_size = rax->alloc_size;
    size_t usable;
 * free the auxiliary data. */
void raxFreeWithCbAndContext(rax *rax,
                             void (*free_callback)(void *item, void *ctx), void *ctx) {
    raxRecursiveFreeWithCtx(rax,rax->head,free_callback,ctx);
    assert(rax->numnodes == 0);
    size_t *alloc_size = rax->alloc_size;
    size_t usable;
