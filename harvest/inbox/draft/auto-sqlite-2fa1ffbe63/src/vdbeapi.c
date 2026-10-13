// AUTO-DRAFT from sqlite/sqlite PR #9de75a1245e3d6a66b8283673719f92830a38c3c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
    sqlite3_mutex_leave(p->db->mutex);
    return SQLITE_RANGE;
  }
  pVar = &p->aVar[i];
  sqlite3VdbeMemRelease(pVar);
  pVar->flags = MEM_Null;
  p->db->errCode = SQLITE_OK;

  /* If the bit corresponding to this variable in Vdbe.expmask is set, then
  ** binding a new value to this variable invalidates the current query plan.
/* …（同文件无关代码省略）… */
  assert( (p->prepFlags & SQLITE_PREPARE_SAVESQL)!=0 || p->expmask==0 );
  assert( (p->expmask & p->smimask)==p->smimask );
  if( p->expmask!=0 && (p->expmask & (i>=31 ? 0x80000000 : (u32)1<<i))!=0 ){
    /* We might avoid a reprepare here if p->smimask is set and the old
    ** value is an integer other than (0,1).  But that is such a corner
    ** case that it does not seem worth the extra code to implement. */
    p->expired = 1;
  }
  return SQLITE_OK;
}

/* …（同文件无关代码省略）… */
        if( sm1!=sm2 ){
          p->expired = 1;
        }
      }else if( (pVar->flags & MEM_Int)==0 || pVar->u.i!=iValue ){
        /* Always expire if the value really is changing */
        p->expired = 1;
      }
    }
