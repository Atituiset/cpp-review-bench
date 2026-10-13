// AUTO-DRAFT from sqlite/sqlite PR #4226522153ecdf8a18c9498e0f12cee469398965
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
** Consider uncorrelated subqueries to be constants if the bAllowSubq
** parameter is true.
*/
static int sqlite3ExprIsTableConstant(Expr *p, int iCur, int bAllowSubq){
  Walker w;
  w.eCode = 3;
  w.pParse = 0;
