// AUTO-DRAFT from sqlite/sqlite PR #3b4484bed2fc51b91dfd4bbe301ba576625e9e34
typedef struct VStatFile VStatFile;
/* …（同文件无关代码省略）… */
static int vstatRegister(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pThunk
){
  return sqlite3_create_module(db, "vfsstat", &VfsStatModule, 0);
}
/* …（同文件无关代码省略）… */
  vstat_vfs.base.szOsFile = sizeof(VStatFile) + vstat_vfs.pVfs->szOsFile;
  rc = sqlite3_vfs_register(&vstat_vfs.base, 1);
  if( rc==SQLITE_OK ){
    rc = vstatRegister(db, pzErrMsg, pApi);  // <<< BUG ANCHOR
    if( rc==SQLITE_OK ){
      rc = sqlite3_auto_extension((void(*)(void))vstatRegister);
    }
  }
  if( rc==SQLITE_OK ) rc = SQLITE_OK_LOAD_PERMANENTLY;
