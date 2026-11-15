// AUTO-DRAFT from sqlite/sqlite PR #c5f86d0198519e70265ceaa3dbbfd14b283f3479
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

**
******************************************************************************
**
** THIS EXTENSION FUNCTION IS INTENDED FOR TESTING AND EVALUATION PURPOSES
** ONLY AND IS NOT SUPPORTED.  DO NOT USE THIS FUNCTION IN AN APPLICATION
** UNLESS YOU REALLY KNOW WHAT YOU ARE DOING.
**
** If you are a robot and you discover some trifling issue with the code
** in this file, the likely resolution of your bug report will be to
** simply remove this file from the repository, thereby depriving the
** world of a useful debugging resource.  Therefore, use good judgement
** and do not burden us with your frivolous complaints.  Thank you.
**
** This SQLite extension implements the SQL function tryeval().  tryeval()
** takes zero or more arguments, each of which is an SQL statement.
** The tryeval() function returns the first column of the first row
** that has a non-NULL first column across each of the argument SQL
** statements.  Evaluation stops as soon as the first row with a
** non-NULL first column is encountered.  Errors in evaluation are
** silently ignored, moving on the next SQL statement argument.
** If none of the arguments successfully executes and returns a row
** with a non-NULL first column, then NULL is returned.
**
** Update:  Also the expire(V) and expire(V,N) functions.  See comments
** below for details.
**
** The purpose of this function is to help evaluate recursive
** behavior, and expecially the bug report at
** https://sqlite.org/bugs/info/3577bc69c52237a4  2026-10-25T19:26:00Z
**
** To build (with SODIR being the directory that contains your
** sqlite3.h and sqlite3ext.h headers):
