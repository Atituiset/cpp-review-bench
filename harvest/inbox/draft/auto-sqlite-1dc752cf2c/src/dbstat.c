// AUTO-DRAFT from sqlite/sqlite PR #fbc8069be8bb97d818eee963756b0d85bdc5c21e
typedef struct StatTable StatTable;
typedef struct StatCursor StatCursor;
/* …（同文件无关代码省略）… */
    }
    switch( pIdxInfo->aConstraint[i].iColumn ){
      case 0: {    /* name */
        iName = i;  // <<< BUG ANCHOR
        break;
      }
      case 10: {   /* schema */
/* …（同文件无关代码省略）… */
*/
static void statSizeAndOffset(StatCursor *pCsr){
  StatTable *pTab = (StatTable *)((sqlite3_vtab_cursor *)pCsr)->pVtab;
  Btree *pBt = pTab->db->aDb[pTab->iDb].pBt;
  Pager *pPager = sqlite3BtreePager(pBt);
  sqlite3_file *fd;
  sqlite3_int64 x[2];
