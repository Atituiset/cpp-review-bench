// AUTO-DRAFT from sqlite/sqlite PR #795e28a89e40246b7029f62440519229109c0503
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
  pA->zNormSql = pB->zNormSql;
  pB->zNormSql = zTmp;
#endif
  pB->expmask |= pA->expmask;
  pB->smimask |= pA->smimask;
  pB->prepFlags = pA->prepFlags;
  memcpy(pB->aCounter, pA->aCounter, sizeof(pB->aCounter));
  pB->aCounter[SQLITE_STMTSTATUS_REPREPARE]++;
