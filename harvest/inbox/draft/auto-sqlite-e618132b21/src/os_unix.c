// AUTO-DRAFT from sqlite/sqlite PR #30be364a7c4fd8e30249ceea7b5054f134656e02
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#    define SQLITE_ENABLE_LOCKING_STYLE 1
/* …（同文件无关代码省略）… */
#define SQLITE_FSFLAGS_IS_MSDOS     0x1
/* …（同文件无关代码省略）… */
# define SQLITE_DEFAULT_FILE_PERMISSIONS 0644
/* …（同文件无关代码省略）… */
#define MAX_PATHNAME 512
/* …（同文件无关代码省略）… */
typedef struct unixInodeInfo unixInodeInfo;   /* An i-node */
/* …（同文件无关代码省略）… */
typedef struct unixFile unixFile;
/* …（同文件无关代码省略）… */
#define UNIXFILE_EXCL        0x01     /* Connections from one process only */
/* …（同文件无关代码省略）… */
#define UNIXFILE_PSOW        0x10     /* SQLITE_IOCAP_POWERSAFE_OVERWRITE */
#define UNIXFILE_DELETE      0x20     /* Delete on close */
#define UNIXFILE_URI         0x40     /* Filename might have query parameters */
#define UNIXFILE_NOLOCK      0x80     /* Do no file locking */
/* …（同文件无关代码省略）… */
#define osOpen      ((int(*)(const char*,int,int))aSyscall[0].pCurrent)
/* …（同文件无关代码省略）… */
#define osClose     ((int(*)(int))aSyscall[1].pCurrent)
/* …（同文件无关代码省略）… */
#define osStat      ((int(*)(const char*,struct stat*))aSyscall[4].pCurrent)
/* …（同文件无关代码省略）… */
#define osFstat(a,b,c)    0
/* …（同文件无关代码省略）… */
#define osFcntl     ((int(*)(int,int,...))aSyscall[7].pCurrent)
/* …（同文件无关代码省略）… */
#define osWrite     ((ssize_t(*)(int,const void*,size_t))aSyscall[11].pCurrent)
/* …（同文件无关代码省略）… */
#define osFchmod    ((int(*)(int,mode_t))aSyscall[14].pCurrent)
/* …（同文件无关代码省略）… */
#define osUnlink    ((int(*)(const char*))aSyscall[16].pCurrent)
/* …（同文件无关代码省略）… */
# define SQLITE_MINIMUM_FILE_DESCRIPTOR 3
/* …（同文件无关代码省略）… */
static int robust_open(const char *z, int f, mode_t m){
  int fd;
  mode_t m2 = m ? m : SQLITE_DEFAULT_FILE_PERMISSIONS;
  while(1){
#if defined(O_CLOEXEC)
    fd = osOpen(z,f|O_CLOEXEC,m2);
#else
    fd = osOpen(z,f,m2);
#endif
    if( fd<0 ){
      if( errno==EINTR ) continue;
      break;
    }
    if( fd>=SQLITE_MINIMUM_FILE_DESCRIPTOR ) break;
    if( (f & (O_EXCL|O_CREAT))==(O_EXCL|O_CREAT) ){
      (void)osUnlink(z);
    }
    osClose(fd);
    sqlite3_log(SQLITE_WARNING,
                "attempt to open \"%s\" as file descriptor %d", z, fd);
    fd = -1;
    if( osOpen("/dev/null", O_RDONLY, m)<0 ) break;
  }
  if( fd>=0 ){
    if( m!=0 ){
      struct stat statbuf;
      if( osFstat(fd, &statbuf)==0
       && statbuf.st_size==0
       && (statbuf.st_mode&0777)!=m
      ){
        osFchmod(fd, m);
      }
    }
#if defined(FD_CLOEXEC) && (!defined(O_CLOEXEC) || O_CLOEXEC==0)
    osFcntl(fd, F_SETFD, osFcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
#endif
  }
  return fd;
}
/* …（同文件无关代码省略）… */
static void unixEnterMutex(void){
  assert( sqlite3_mutex_notheld(unixBigLock) );  /* Not a recursive mutex */
  sqlite3_mutex_enter(unixBigLock);
}
static void unixLeaveMutex(void){
  assert( sqlite3_mutex_held(unixBigLock) );
  sqlite3_mutex_leave(unixBigLock);
}
/* …（同文件无关代码省略）… */
static int unixMutexHeld(void) {
  return sqlite3_mutex_held(unixBigLock);
}
/* …（同文件无关代码省略）… */
struct vxworksFileId {
  struct vxworksFileId *pNext;  /* Next in a list of them all */
  int nRef;                     /* Number of references to this one */
  int nName;                    /* Length of the zCanonicalName[] string */
  char *zCanonicalName;         /* Canonical filename */
};
/* …（同文件无关代码省略）… */
static int vxworksSimplifyName(char *z, int n){
  int i, j;
  while( n>1 && z[n-1]=='/' ){ n--; }
  for(i=j=0; i<n; i++){
    if( z[i]=='/' ){
      if( z[i+1]=='/' ) continue;
      if( z[i+1]=='.' && i+2<n && z[i+2]=='/' ){
        i += 1;
        continue;
      }
      if( z[i+1]=='.' && i+3<n && z[i+2]=='.' && z[i+3]=='/' ){
        while( j>0 && z[j-1]!='/' ){ j--; }
        if( j>0 ){ j--; }
        i += 2;
        continue;
      }
    }
    z[j++] = z[i];
  }
  z[j] = 0;
  return j;
}
/* …（同文件无关代码省略）… */
static struct vxworksFileId *vxworksFindFileId(const char *zAbsoluteName){
  struct vxworksFileId *pNew;         /* search key and new file ID */
  struct vxworksFileId *pCandidate;   /* For looping over existing file IDs */
  int n;                              /* Length of zAbsoluteName string */

  assert( zAbsoluteName[0]=='/' );
  n = (int)strlen(zAbsoluteName);
  pNew = sqlite3_malloc64( sizeof(*pNew) + (n+1) );
  if( pNew==0 ) return 0;
  pNew->zCanonicalName = (char*)&pNew[1];
  memcpy(pNew->zCanonicalName, zAbsoluteName, n+1);
  n = vxworksSimplifyName(pNew->zCanonicalName, n);

  /* Search for an existing entry that matching the canonical name.
  ** If found, increment the reference count and return a pointer to
  ** the existing file ID.
  */
  sqlite3_mutex_enter(vxworksMutex);
  for(pCandidate=vxworksFileList; pCandidate; pCandidate=pCandidate->pNext){
    if( pCandidate->nName==n
     && memcmp(pCandidate->zCanonicalName, pNew->zCanonicalName, n)==0
    ){
       sqlite3_free(pNew);
       pCandidate->nRef++;
       sqlite3_mutex_leave(vxworksMutex);
       return pCandidate;
    }
  }

  /* No match was found.  We will make a new file ID */
  pNew->nRef = 1;
  pNew->nName = n;
  pNew->pNext = vxworksFileList;
  vxworksFileList = pNew;
  sqlite3_mutex_leave(vxworksMutex);
  return pNew;
}
/* …（同文件无关代码省略）… */
static void vxworksReleaseFileId(struct vxworksFileId *pId){
  sqlite3_mutex_enter(vxworksMutex);
  assert( pId->nRef>0 );
  pId->nRef--;
  if( pId->nRef==0 ){
    struct vxworksFileId **pp;
    for(pp=&vxworksFileList; *pp && *pp!=pId; pp = &((*pp)->pNext)){}
    assert( *pp==pId );
    *pp = pId->pNext;
    sqlite3_free(pId);
  }
  sqlite3_mutex_leave(vxworksMutex);
}
/* …（同文件无关代码省略）… */
struct unixFileId {
  dev_t dev;                  /* Device number */
#if OS_VXWORKS
  struct vxworksFileId *pId;  /* Unique file ID for vxworks. */
#else
  /* We are told that some versions of Android contain a bug that
  ** sizes ino_t at only 32-bits instead of 64-bits. (See
  ** https://android-review.googlesource.com/#/c/115351/3/dist/sqlite3.c)
  ** To work around this, always allocate 64-bits for the in
