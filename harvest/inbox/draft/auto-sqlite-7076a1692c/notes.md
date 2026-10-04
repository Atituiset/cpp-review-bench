# auto-sqlite-7076a1692c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#49437882edaf145572706daccd79194acb160843](https://github.com/sqlite/sqlite/commit/49437882edaf145572706daccd79194acb160843) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #49437882edaf145572706daccd79194acb160843 (https://github.com/sqlite/sqlite/commit/49437882edaf145572706daccd79194acb160843)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 49437882edaf145572706daccd79194acb160843 Enhance the sqlite3_incomplete() interface so that it returns SQLITE_EMPTY :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -51,10 +51,10 @@ extern const char sqlite3IsEbcdicIdChar[];
 /*
 ** Return zero if the given SQL string is complete - if all comments,
 ** string and blob literals, and quoted identifiers have been closed and
-** if the entire string ends with ";" and possible with ";END;" if the
+** if the entire string ends with ";" and possibly with ";END;" if the
 ** string is a CREATE TRIGGER statement.  A non-zero return indicates
-** that the string is incomplete.  Bits of the return value indicate
-** what is missing and is needed to close out the statement.
+** that the string is empty or incomplete.  Bits of the return value
+** indicate what is missing and is needed to close out the statement.
 **
 ** Special handling is require for CREATE TRIGGER statements.
 ** Whenever the CREATE TRIGGER keywords are seen, the statement
@@ -66,11 +66,12 @@ extern const char sqlite3IsEbcdicIdChar[];
 **    R = 0xwwwwwwww00xxyyzz
 **
 ** In other words, zz is the least significant byte, yy is the next
-** most significant byte, xx is the third byte, wwwwwwww is a 32-bit
-** value from the middle.
+** most significant byte, xx is the third byte, wwwwwwww is the upper
+** four bytes of the return value.
 **
 **   zz == SQLITE_OK       Input is complete
 **   zz == SQLITE_ERROR    Input is incomplete
+**   zz == SQLITE_EMPTY    Input contains only whitespace
 **   zz == SQLITE_MISUSE   Input is a NULL pointer
 **   zz != 0               New values for zz may be added in the future
 **
@@ -96,7 +97,7 @@ extern const char sqlite3IsEbcdicIdChar[];
 **
 ** This implementation uses a state machine with 8 states:
 **
-**   (0) INVALID   We have not yet seen a non-whitespace character.
+**   (0) NOTHING   We have not yet seen a non-whitespace character.
 **
 **   (1) START     At the beginning or end of an SQL statement.  This routine
 **                 returns 1 if it ends in the START state and 0 if it ends
@@ -153,7 +154,7 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
   static const u8 trans[8][8] = {
                      /* Token:                                                */
      /* State:       **  SEMI  WS  OTHER  EXPLAIN  CREATE  TEMP  TRIGGER  END */
-     /* 0 INVALID: */ {    1,  0,     2,       3,      4,    2,       2,   2, },
+     /* 0 NOTHING: */ {    1,  0,     2,       3,      4,    2,       2,   2, },
      /* 1   START: */ {    1,  1,     2,       3,      4,    2,       2,   2, },
      /* 2  NORMAL: */ {    1,  2,     2,       2,      2,    2,       2,   2, },
      /* 3 EXPLAIN: */ {    1,  3,     3,       2,      4,    2,       2,   2, },
@@ -169,14 +170,14 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
   static const u8 trans[3][3] = {
                      /* Token:           */
      /* State:       **  SEMI  WS  OTHER */
-     /* 0 INVALID: */ {    1,  0,     2, },
+     /* 0 NOTHING: */ {    1,  0,     2, },
      /* 1   START: */ {    1,  1,     2, },
      /* 2  NORMAL: */ {    1,  2,     2, },
   };
 #endif /* SQLITE_OMIT_TRIGGER */
   /* Mapping state number to yy value for the return */
   static const u8 statemap[8] = {
-     /* 0 INVALID */ 1,
+     /* 0 NOTHING */ 0,
      /* 1 START   */ 0,
      /* 2 NORMAL  */ 1,
      /* 3 EXPLAIN */ 1,
@@ -214,6 +215,7 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
         zSql += 2;
         while( zSql[0] && (zSql[0]!='*' || zSql[1]!='/') ){ zSql++; }
         if( zSql[0]==0 ){
+          if( state==0 ) state = 2;
           pending = '/';
           goto incomplete_finish;
         }
@@ -331,6 +333,7 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
     zSql++;
   }
 incomplete_finish:
+  if( state==0 ) return SQLITE_EMPTY;
   if( state==1 ) nParen = 0;
   return (i64)((((u64)nParen)<<32) |
                ((u64)pending<<16) |
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`CREATE`
- 大写宏：`END`
- 大写宏：`EXPLAIN`
- 大写宏：`INVALID`
- 大写宏：`NORMAL`
- 大写宏：`NULL`
- 大写宏：`OTHER`
- 大写宏：`SEMI`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_MISUSE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_TRIGGER`
- 大写宏：`START`
- 大写宏：`TEMP`
- 大写宏：`TRIGGER`
- 外部类型：`At`
- 外部类型：`Bits`
- 外部类型：`In`
- 外部类型：`Input`
- 外部类型：`Mapping`
- 外部类型：`New`
- 外部类型：`Return`
- 外部类型：`Special`
- 外部类型：`State`
- 外部类型：`This`
- 外部类型：`Token`
- 外部类型：`We`
- 外部类型：`Whenever`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7076a1692c` → 本草稿移入 `cases/defect/auto-sqlite-7076a1692c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
