// AUTO-DRAFT from sqlite/sqlite PR #1820defd768c50e0ea33476c9b396de7c2c60bc4
    int nPrefix;                  /* Prefix length (0 for main terms index) */
    Fts3Hash hPending;            /* Pending terms table for this index */
  } *aIndex;
  int nMaxPendingData;            /* Max pending data before flush to disk */  // <<< BUG ANCHOR
  int nPendingData;               /* Current bytes of pending data */
  sqlite_int64 iPrevDocid;        /* Docid of most recently inserted document */
  int iPrevLangid;                /* Langid of recently inserted document */
  int bPrevDelete;                /* True if last operation was a delete */
