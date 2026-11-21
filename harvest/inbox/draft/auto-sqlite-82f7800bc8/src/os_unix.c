// AUTO-DRAFT from sqlite/sqlite PR #476e16ed1c9cc93d00782197bbfb3853f903191f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
struct vxworksFileId {
  struct vxworksFileId *pNext;  /* Next in a list of them all */
  int nRef;                     /* Number of references to this one */
  int nName;                    /* Length of the zCanonicalName[] string */
  char *zCanonicalName;         /* Canonical filename */
};
/* …（同文件无关代码省略）… */
  struct vxworksFileId *pCandidate;   /* For looping over existing file IDs */
  int n;                              /* Length of zAbsoluteName string */

  if( zAbsoluteName==0 || zAbsoluteName[0]=='/' );
  n = (int)strlen(zAbsoluteName);
  pNew = sqlite3_malloc64( sizeof(*pNew) + (n+1) );
  if( pNew==0 ) return 0;
