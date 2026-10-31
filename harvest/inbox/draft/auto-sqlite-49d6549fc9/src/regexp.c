// AUTO-DRAFT from sqlite/sqlite PR #47b4187a62d3fabc64cc9d779546eba9c29e233b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
#define re_free    sqlite3re_free
/* …（同文件无关代码省略）… */
#define RE_OP_ANYSTAR     3    /* Special optimized version of .* */
/* …（同文件无关代码省略）… */
typedef struct ReCompiled ReCompiled;
/* …（同文件无关代码省略）… */
static int re_resize(ReCompiled *p, unsigned int N){
  char *aOp;
  int *aArg;
  if( N>p->mxAlloc ){ p->zErr = "REGEXP pattern too big"; return 1; }
  aOp = sqlite3_realloc64(p->aOp, N*sizeof(p->aOp[0]));
  if( aOp==0 ){ p->zErr = "out of memory"; return 1; }
  p->aOp = aOp;
  aArg = sqlite3_realloc64(p->aArg, N*sizeof(p->aArg[0]));
  if( aArg==0 ){ p->zErr = "out of memory"; return 1; }
  p->aArg = aArg;
  p->nAlloc = N;
  return 0;
}
/* …（同文件无关代码省略）… */
static int re_insert(ReCompiled *p, int iBefore, int op, int arg){
  int i;
  if( p->nAlloc<=p->nState && re_resize(p, p->nAlloc*2) ) return 0;
  for(i=p->nState; i>iBefore; i--){
    p->aOp[i] = p->aOp[i-1];
    p->aArg[i] = p->aArg[i-1];
  }
  p->nState++;
  p->aOp[iBefore] = (char)op;
  p->aArg[iBefore] = arg;
  return iBefore;
}
/* …（同文件无关代码省略）… */
static int re_append(ReCompiled *p, int op, int arg){
  return re_insert(p, p->nState, op, arg);
}
/* …（同文件无关代码省略）… */
    re_free(pRe);
    return zErr;
  }
  if( zIn[0]=='^' ){
    zIn++;
  }else{
    re_append(pRe, RE_OP_ANYSTAR, 0);
  }
  pRe->sIn.z = (unsigned char*)zIn;
  pRe->sIn.i = 0;
  pRe->sIn.mx = (int)strlen(zIn);
/* …（同文件无关代码省略）… */
    return "unrecognized character";
  }

  /* The following is a performance optimization.  If the regex begins with
  ** ".*" (if the input regex lacks an initial "^") and afterwards there are
  ** one or more matching characters, enter those matching characters into
