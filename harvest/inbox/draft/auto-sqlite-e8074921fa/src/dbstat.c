// AUTO-DRAFT from sqlite/sqlite PR #9b0f74e9615f1c79e43fa876b84f8a5952030e8a
# define get2byte(x)   ((x)[0]<<8 | (x)[1])
/* …（同文件无关代码省略）… */
  p->nMxPayload = 0;
  szPage = sqlite3BtreeGetPageSize(pBt);
  // <<< BUG ANCHOR
  nUnused = get2byte(&aHdr[5]) - nHdr - 2*p->nCell;
  nUnused += (int)aHdr[7];
  iOff = get2byte(&aHdr[1]);
  while( iOff ){
