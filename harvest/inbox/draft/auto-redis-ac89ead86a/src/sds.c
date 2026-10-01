// AUTO-DRAFT from redis/redis PR #14927
x = sdsResize(x, 200, 1);
        test_cond("sdsresize() expand len", sdslen(x) == 40);
        test_cond("sdsresize() expand strlen", strlen(x) == 40);
        test_cond("sdsresize() expand alloc", sdsalloc(x) == 200);  // <<< BUG ANCHOR
        /* Test sdsresize - trim free space */
        x = sdsResize(x, 80, 1);
        test_cond("sdsresize() shrink len", sdslen(x) == 40);
        test_cond("sdsresize() shrink strlen", strlen(x) == 40);
        test_cond("sdsresize() shrink alloc", sdsalloc(x) == 80);
        /* Test sdsresize - crop used space */
        x = sdsResize(x, 30, 1);
        test_cond("sdsresize() crop len", sdslen(x) == 30);
        test_cond("sdsresize() crop strlen", strlen(x) == 30);
        test_cond("sdsresize() crop alloc", sdsalloc(x) == 30);
        /* Test sdsresize - extend to different class */
        x = sdsResize(x, 400, 1);
        test_cond("sdsresize() expand len", sdslen(x) == 30);
        test_cond("sdsresize() expand strlen", strlen(x) == 30);
        test_cond("sdsresize() expand alloc", sdsalloc(x) == 400);
        /* Test sdsresize - shrink to different class */
        x = sdsResize(x, 4, 1);
        test_cond("sdsresize() crop len", sdslen(x) == 4);
        test_cond("sdsresize() crop strlen", strlen(x) == 4);
        test_cond("sdsresize() crop alloc", sdsalloc(x) == 4);
        sdsfree(x);
        
        { /* Test adjustTypeIfNeeded() */
