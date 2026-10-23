// AUTO-DRAFT from sqlite/sqlite PR #20822819adf299e3a7609a26206bf2b43ac402ff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
# define ALWAYS(X)      (1)
/* …（同文件无关代码省略）… */
typedef struct Rtree Rtree;
/* …（同文件无关代码省略）… */
typedef struct RtreeNode RtreeNode;
/* …（同文件无关代码省略）… */
typedef struct RtreeSearchPoint RtreeSearchPoint;
/* …（同文件无关代码省略）… */
#define HASHSIZE 97
/* …（同文件无关代码省略）… */
# define RTREE_IS_CORRUPT(X) ((X)->bCorrupt = 1)
/* …（同文件无关代码省略）… */
#define RTREE_MAX_DEPTH 40
/* …（同文件无关代码省略）… */
#define NCELL(pNode) readInt16(&(pNode)->zData[2])
/* …（同文件无关代码省略）… */
static int readInt16(u8 *p){
  return (p[0]<<8) + p[1];
}
/* …（同文件无关代码省略）… */
static void nodeReference(RtreeNode *p){
  if( p ){
    assert( p->nRef>0 );
    p->nRef++;
  }
}
/* …（同文件无关代码省略）… */
static unsigned int nodeHash(i64 iNode){
  return ((unsigned)iNode) % HASHSIZE;
}
/* …（同文件无关代码省略）… */
static RtreeNode *nodeHashLookup(Rtree *pRtree, i64 iNode){
  RtreeNode *p;
  for(p=pRtree->aHash[nodeHash(iNode)]; p && p->iNode!=iNode; p=p->pNext);
  return p;
}
/* …（同文件无关代码省略）… */
static void nodeHashInsert(Rtree *pRtree, RtreeNode *pNode){
  int iHash;
  assert( pNode->pNext==0 );
  iHash = nodeHash(pNode->iNode);
  pNode->pNext = pRtree->aHash[iHash];
  pRtree->aHash[iHash] = pNode;
}
/* …（同文件无关代码省略）… */
static void nodeBlobReset(Rtree *pRtree){
  sqlite3_blob *pBlob = pRtree->pNodeBlob;
  pRtree->pNodeBlob = 0;
  sqlite3_blob_close(pBlob);
}
/* …（同文件无关代码省略）… */
static int nodeAcquire(
  Rtree *pRtree,             /* R-tree structure */
  i64 iNode,                 /* Node number to load */
  RtreeNode *pParent,        /* Either the parent node or NULL */
  RtreeNode **ppNode         /* OUT: Acquired node */
){
  int rc = SQLITE_OK;
  RtreeNode *pNode = 0;

  /* Check if the requested node is already in the hash table. If so,
  ** increase its reference count and return it.
  */
  if( (pNode = nodeHashLookup(pRtree, iNode))!=0 ){
    if( pParent && ALWAYS(pParent!=pNode->pParent) ){
      RTREE_IS_CORRUPT(pRtree);
      return SQLITE_CORRUPT_VTAB;
    }
    pNode->nRef++;
    *ppNode = pNode;
    return SQLITE_OK;
  }

  if( pRtree->pNodeBlob ){
    sqlite3_blob *pBlob = pRtree->pNodeBlob;
    pRtree->pNodeBlob = 0;
    rc = sqlite3_blob_reopen(pBlob, iNode);
    pRtree->pNodeBlob = pBlob;
    if( rc ){
      nodeBlobReset(pRtree);
      if( rc==SQLITE_NOMEM ) return SQLITE_NOMEM;
    }
  }
  if( pRtree->pNodeBlob==0 ){
    rc = sqlite3_blob_open(pRtree->db, pRtree->zDb, pRtree->zNodeName,
                           "data", iNode, 0,
                           &pRtree->pNodeBlob);
  }
  if( rc ){
    *ppNode = 0;
    /* If unable to open an sqlite3_blob on the desired row, that can only
    ** be because the shadow tables hold erroneous data. */
    if( rc==SQLITE_ERROR ){
      rc = SQLITE_CORRUPT_VTAB;
      RTREE_IS_CORRUPT(pRtree);
    }
  }else if( iNode<=0 ){
    RTREE_IS_CORRUPT(pRtree);
    rc = SQLITE_CORRUPT_VTAB;
  }else if( pRtree->iNodeSize==sqlite3_blob_bytes(pRtree->pNodeBlob) ){
    pNode = (RtreeNode *)sqlite3_malloc64(sizeof(RtreeNode)+pRtree->iNodeSize);
    if( !pNode ){
      rc = SQLITE_NOMEM;
    }else{
      pNode->pParent = pParent;
      pNode->zData = (u8 *)&pNode[1];
      pNode->nRef = 1;
      pRtree->nNodeRef++;
      pNode->iNode = iNode;
      pNode->isDirty = 0;
      pNode->pNext = 0;
      rc = sqlite3_blob_read(pRtree->pNodeBlob, pNode->zData,
                             pRtree->iNodeSize, 0);
    }
  }

  /* If the root node was just loaded, set pRtree->iDepth to the height
  ** of the r-tree structure. A height of zero means all data is stored on
  ** the root node. A height of one means the children of the root node
  ** are the leaves, and so on. If the depth as specified on the root node
  ** is greater than RTREE_MAX_DEPTH, the r-tree structure must be corrupt.
  */
  if( rc==SQLITE_OK && pNode && iNode==1 ){
    pRtree->iDepth = readInt16(pNode->zData);
    if( pRtree->iDepth>=RTREE_MAX_DEPTH ){
      rc = SQLITE_CORRUPT_VTAB;
      RTREE_IS_CORRUPT(pRtree);
    }
  }

  /* If no error has occurred so far, check if the "number of entries"
  ** field on the node is too large. If so, set the return code to 
  ** SQLITE_CORRUPT_VTAB.
  */
  if( pNode && rc==SQLITE_OK ){
    if( NCELL(pNode)>((pRtree->iNodeSize-4)/pRtree->nBytesPerCell) ){
      rc = SQLITE_CORRUPT_VTAB;
      RTREE_IS_CORRUPT(pRtree);
    }
  }

  if( rc==SQLITE_OK ){
    if( pNode!=0 ){
      nodeReference(pParent);
      nodeHashInsert(pRtree, pNode);
    }else{
      rc = SQLITE_CORRUPT_VTAB;
      RTREE_IS_CORRUPT(pRtree);
    }
    *ppNode = pNode;
  }else{
    nodeBlobReset(pRtree);
    if( pNode ){
      pRtree->nNodeRef--;
      sqlite3_free(pNode);
    }
    *ppNode = 0;
  }

  return rc;
}
/* …（同文件无关代码省略）… */
static int findLeafNode(
  Rtree *pRtree,              /* RTree to search */
  i64 iRowid,                 /* The rowid searching for */
  RtreeNode **ppLeaf,         /* Write the node here */
  sqlite3_int64 *piNode       /* Write the node-id here */
){
  int rc;
  *ppLeaf = 0;
  sqlite3_bind_int64(pRtree->pReadRowid, 1, iRowid);
  if( sqlite3_step(pRtree->pReadRowid)==SQLITE_ROW ){
    i64 iNode = sqlite3_column_int64(pRtree->pReadRowid, 0);
    if( piNode ) *piNode = iNode;
    rc = nodeAcquire(pRtree, iNode, 0, ppLeaf);
    sqlite3_reset(pRtree->pReadRowid);
  }else{
    rc = sqlite3_reset(pRtree->pReadRowid);
  }
  return rc;
}
/* …（同文件无关代码省略）… */

int sqlite3IntFloatCompare(i64,double);

/* 
** Rtree virtual table module xFilter method.
*/
/* …（同文件无关代码省略）… */
    /* Special case - lookup by rowid. */
    RtreeNode *pLeaf;        /* Leaf on which the required cell resides */
    RtreeSearchPoint *p;     /* Search point for the leaf */
    i64 iRowid = sqlite3_value_int64(argv[0]);
    i64 iNode = 0;
    int eType = sqlite3_value_numeric_type(argv[0]);
    if( eType==SQLITE_INTEGER
     || (eType==SQLITE_FLOAT 
         && 0==sqlite3IntFlo
