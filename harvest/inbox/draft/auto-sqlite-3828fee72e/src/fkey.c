// AUTO-DRAFT from sqlite/sqlite PR #17f128f7466f21396e05591067c36ad8dbbb0f3d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
** to trigger pFKey.
*/
static int isSetNullAction(Parse *pParse, FKey *pFKey){
  Parse *pTop = sqlite3ParseToplevel(pParse);
  if( pTop->pTriggerPrg ){
    Trigger *p = pTop->pTriggerPrg->pTrigger;
    if( (p==pFKey->apTrigger[0] && pFKey->aAction[0]==OE_SetNull)
     || (p==pFKey->apTrigger[1] && pFKey->aAction[1]==OE_SetNull)
    ){
      assert( (pTop->db->flags & SQLITE_FkNoAction)==0 );
      return 1;
    }
  }
