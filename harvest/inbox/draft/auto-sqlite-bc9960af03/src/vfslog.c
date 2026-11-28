// AUTO-DRAFT from sqlite/sqlite PR #a2c156076e1f86a8373daaadf7e77f5f0f03602e
typedef struct VLogFile VLogFile;
/* …（同文件无关代码省略）… */
** Register debugvfs as the default VFS for this process.
*/
int sqlite3_register_vfslog(const char *zArg){
  vlog_vfs.pVfs = sqlite3_vfs_find(0);  // <<< BUG ANCHOR
  if( vlog_vfs.pVfs==0 ) return SQLITE_ERROR;
  vlog_vfs.base.szOsFile = sizeof(VLogFile) + vlog_vfs.pVfs->szOsFile;
  return sqlite3_vfs_register(&vlog_vfs.base, 1);
}
