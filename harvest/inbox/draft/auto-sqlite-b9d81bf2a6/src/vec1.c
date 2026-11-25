// AUTO-DRAFT from sqlite/sqlite PR #803d64f73ba09b9a0ea338b5c621493ac857d1e3
#define VEC1_MODEL_RESIDUAL 0x04
/* …（同文件无关代码省略）… */
#define VEC1_MAX_BUCKET            65536
/* …（同文件无关代码省略）… */
#define VEC1_QUANTIZE_NONE 0
/* …（同文件无关代码省略）… */
#define VEC1_QUANTIZE_BQ   3
/* …（同文件无关代码省略）… */
    ** Transform for binary quantizers. Disable it for other quantizers.  */
    if( p->bWHT<0 ) p->bWHT = (p->eQuant==VEC1_QUANTIZE_BQ);

    /* If no value was specified for "codesize", choose a default. This
    ** is 32 for PQ or OPQ, 0 for "none" quantizer, and (nElem+7)/8 for BQ. */
    if( p->eQuant==VEC1_QUANTIZE_NONE ){
/* …（同文件无关代码省略）… */
   || pMod->hdr.nBucket>VEC1_MAX_BUCKET 
   || pMod->hdr.nCodebook>pMod->hdr.nElem
   || ((pMod->hdr.flags & VEC1_MODEL_RESIDUAL) && pMod->hdr.nBucket==0)
  ){
    *pzErr = sqlite3_mprintf("vec1: model is corrupt");
    return SQLITE_ERROR;
