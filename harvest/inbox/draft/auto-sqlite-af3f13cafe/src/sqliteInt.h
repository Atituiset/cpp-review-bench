// AUTO-DRAFT from sqlite/sqlite PR #d85a80711cfef59a0742362a9b2ee5c70f5b8267
  int nErr;            /* Number of errors seen */
  int nTab;            /* Number of previously allocated VDBE cursors */
  int nMem;            /* Number of memory cells used so far */
  int iSelfTab;        /* Table associated with an index on expr, or negative
                       ** of the base register during check-constraint eval */
  int nNestSel;        /* Number of nested SELECT statements and/or VIEWs */
