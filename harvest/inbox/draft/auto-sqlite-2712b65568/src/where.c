// AUTO-DRAFT from sqlite/sqlite PR #4132f00833c5b7596167ef62bc15ca4b40bcb330
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

      }
    }
    if( pLevel->addrNxt!=pLevel->addrBrk ){
      /* This occurs in two cases - (a) for a LIKE/GLOB scan that must visit
      ** ranges of text and blob values separately, and (b) when there is
      ** an IN(...) constraint driving the index. This is asserted() in the
      ** next block, inside the #ifndef. */
      sqlite3VdbeResolveLabel(v, pLevel->addrNxt);
    }
#ifndef SQLITE_LIKE_DOESNT_MATCH_BLOBS
    assert( (pLevel->addrNxt==pLevel->addrBrk)
         || (pLevel->addrLikeRep)                                    /* (a) */
         || ((pLoop->wsFlags & WHERE_IN_ABLE) && pLevel->u.in.nIn>0) /* (b) */
    );
    if( pLevel->addrLikeRep ){
      /* The LIKE optimization loop is nested inside any IN(...) and
