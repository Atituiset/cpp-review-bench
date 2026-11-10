// AUTO-DRAFT from sqlite/sqlite PR #5909e4f54d42746463186e702fdf63395056844c
    }else{
      sqlite3_bind_null(p, 1);
    }
    sqlite3_bind_blob(p, 2, pNode->zData, pRtree->iNodeSize, SQLITE_STATIC);  // <<< BUG ANCHOR
    sqlite3_step(p);
    pNode->isDirty = 0;
    rc = sqlite3_reset(p);
