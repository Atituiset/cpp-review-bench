// AUTO-DRAFT from sqlite/sqlite PR #80a6cb85686e6fabaab23b318bce66087e782b86
typedef struct NodeWriter NodeWriter;
typedef struct Blob Blob;
/* …（同文件无关代码省略）… */
** unmodified. Otherwise, if the allocation succeeds, update pBlob->nAlloc
** to reflect the new size of the pBlob->a[] buffer.
*/
static void blobGrowBuffer(Blob *pBlob, int nMin, int *pRc){  // <<< BUG ANCHOR
  if( *pRc==SQLITE_OK && nMin>pBlob->nAlloc ){
    int nAlloc = nMin;
    char *a = (char *)sqlite3_realloc64(pBlob->a, nAlloc);
    if( a ){
      pBlob->nAlloc = nAlloc;
/* …（同文件无关代码省略）… */
  const char *aDoclist = pCsr->aDoclist;
  int nDoclist = pCsr->nDoclist;
  int rc = SQLITE_OK;           /* Return code */
  int nSpace;                   /* Total space in bytes required on leaf */
  int nPrefix;                  /* Size of prefix shared with previous term */
  int nSuffix;                  /* Size of suffix (nTerm - nPrefix) */
  NodeWriter *pLeaf;            /* Object used to write leaf nodes */
/* …（同文件无关代码省略）… */
  nSuffix = nTerm - nPrefix;
  if(nSuffix<=0 ) return FTS_CORRUPT_VTAB;

  nSpace  = sqlite3Fts3VarintLen(nPrefix);
  nSpace += sqlite3Fts3VarintLen(nSuffix) + nSuffix;
  nSpace += sqlite3Fts3VarintLen(nDoclist) + nDoclist;

  /* If the current block is not empty, and if adding this term/doclist
  ** to the current block would make it larger than Fts3Table.nNodeSize bytes,
