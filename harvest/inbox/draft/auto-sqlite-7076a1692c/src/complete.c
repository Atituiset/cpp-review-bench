// AUTO-DRAFT from sqlite/sqlite PR #49437882edaf145572706daccd79194acb160843
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

/*
** Return zero if the given SQL string is complete - if all comments,
** string and blob literals, and quoted identifiers have been closed and
** if the entire string ends with ";" and possible with ";END;" if the
** string is a CREATE TRIGGER statement.  A non-zero return indicates
** that the string is incomplete.  Bits of the return value indicate
** what is missing and is needed to close out the statement.
**
** Special handling is require for CREATE TRIGGER statements.
** Whenever the CREATE TRIGGER keywords are seen, the statement
/* …（同文件无关代码省略）… */
**    R = 0xwwwwwwww00xxyyzz
**
** In other words, zz is the least significant byte, yy is the next
** most significant byte, xx is the third byte, wwwwwwww is a 32-bit
** value from the middle.
**
**   zz == SQLITE_OK       Input is complete
**   zz == SQLITE_ERROR    Input is incomplete
**   zz == SQLITE_MISUSE   Input is a NULL pointer
**   zz != 0               New values for zz may be added in the future
**
/* …（同文件无关代码省略）… */
**
** This implementation uses a state machine with 8 states:
**
**   (0) INVALID   We have not yet seen a non-whitespace character.
**
**   (1) START     At the beginning or end of an SQL statement.  This routine
**                 returns 1 if it ends in the START state and 0 if it ends
/* …（同文件无关代码省略）… */
  static const u8 trans[8][8] = {
                     /* Token:                                                */
     /* State:       **  SEMI  WS  OTHER  EXPLAIN  CREATE  TEMP  TRIGGER  END */
     /* 0 INVALID: */ {    1,  0,     2,       3,      4,    2,       2,   2, },
     /* 1   START: */ {    1,  1,     2,       3,      4,    2,       2,   2, },
     /* 2  NORMAL: */ {    1,  2,     2,       2,      2,    2,       2,   2, },
     /* 3 EXPLAIN: */ {    1,  3,     3,       2,      4,    2,       2,   2, },
/* …（同文件无关代码省略）… */
  static const u8 trans[3][3] = {
                     /* Token:           */
     /* State:       **  SEMI  WS  OTHER */
     /* 0 INVALID: */ {    1,  0,     2, },
     /* 1   START: */ {    1,  1,     2, },
     /* 2  NORMAL: */ {    1,  2,     2, },
  };
#endif /* SQLITE_OMIT_TRIGGER */
  /* Mapping state number to yy value for the return */
  static const u8 statemap[8] = {
     /* 0 INVALID */ 1,
     /* 1 START   */ 0,
     /* 2 NORMAL  */ 1,
     /* 3 EXPLAIN */ 1,
/* …（同文件无关代码省略）… */
        zSql += 2;
        while( zSql[0] && (zSql[0]!='*' || zSql[1]!='/') ){ zSql++; }
        if( zSql[0]==0 ){
          pending = '/';
          goto incomplete_finish;
        }
/* …（同文件无关代码省略）… */
    zSql++;
  }
incomplete_finish:
  if( state==1 ) nParen = 0;
  return (i64)((((u64)nParen)<<32) |
               ((u64)pending<<16) |
