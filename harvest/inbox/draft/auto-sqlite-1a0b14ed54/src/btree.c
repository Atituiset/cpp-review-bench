// AUTO-DRAFT from sqlite/sqlite PR #0a4a0aab8cf96a9a11d6e7449d5c5e3aee896aa2
int corruptPageError(int lineno, MemPage *p){
  char *zMsg;
  sqlite3BeginBenignMalloc();
  zMsg = sqlite3_mprintf("database corruption page %u of %s",
             p->pgno, sqlite3PagerFilename(p->pBt->pPager, 0)
  );
  sqlite3EndBenignMalloc();
  if( zMsg ){
    sqlite3ReportError(SQLITE_CORRUPT, lineno, zMsg);
  }
  sqlite3_free(zMsg);
  return SQLITE_CORRUPT_BKPT;
}
# define SQLITE_CORRUPT_PAGE(pMemPage) corruptPageError(__LINE__, pMemPage)
/* …（同文件无关代码省略）… */

/*
** Do additional sanity check after btreeInitPage() if
** PRAGMA cell_size_check=ON
*/
static SQLITE_NOINLINE int btreeCellSizeCheck(MemPage *pPage){
  int iCellFirst;    /* First allowable cell or freeblock offset */
/* …（同文件无关代码省略）… */
    testcase( pc==iCellFirst );
    testcase( pc==iCellLast );
    if( pc<iCellFirst || pc>iCellLast ){
      return SQLITE_CORRUPT_PAGE(pPage);
    }
    sz = pPage->xCellSize(pPage, &data[pc]);
    testcase( pc+sz==usableSize );
    if( pc+sz>usableSize ){
      return SQLITE_CORRUPT_PAGE(pPage);
    }
  }
