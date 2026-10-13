// AUTO-DRAFT from sqlite/sqlite PR #77b2a47a202f83d5a48aa259f62346ce497af122
**     SQLITE_MISUSE       Bad arguments
*/
int sqlite3_optimization_mask(const char *zName, sqlite3_uint64 *pMask){
  int i;
  if( zName==0 || pMask==0 ){ return SQLITE_MISUSE; }
  for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
    if( sqlite3_stricmp(zName, aOpt[i].zName)==0 ){
/* …（同文件无关代码省略）… */
** Given a bitmap, return the canonical symbolic name.  Return 0 for
** a bitmap input of 0 or a bitmap value that matches nothing.
*/
const char *sqlite3_optimization_name(sqlite3_int64 mask){  // <<< BUG ANCHOR
  int i;
  for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
    if( aOpt[i].uMask==mask ){ return aOpt[i].zName; }
  }
