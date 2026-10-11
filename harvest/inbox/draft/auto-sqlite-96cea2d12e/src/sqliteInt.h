// AUTO-DRAFT from sqlite/sqlite PR #6e35c19f170958b1e846e01c55318988f2d192e0
  int nErr;            /* Number of errors seen */
  int nTab;            /* Number of previously allocated VDBE cursors */
  int nMem;            /* Number of memory cells used so far */
  int iSelfTab;        /* Table associated with an index on expr, or negative
                       ** of the base register during check-constraint eval */
  int nNestSel;        /* Number of nested SELECT statements and/or VIEWs */
