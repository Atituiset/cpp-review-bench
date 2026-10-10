# auto-sqlite-e670b6f7d7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#012589ecabb5debd4b9a37e96da027e4f41b8838](https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 219 |
| 编译错误数（gcc syntax-only） | 327（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #012589ecabb5debd4b9a37e96da027e4f41b8838 (https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 480；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 012589ecabb5debd4b9a37e96da027e4f41b8838 Bring the tree-tracker branch up-to-date with trunk. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -382,22 +382,7 @@ static int pclose2(FILE *pIn, FILE *pOut, int childPid){
 **    5      First byte of a 4-byte UTF-8
 */
 static const char aSafeChar[256] = {
-#ifdef _WIN32
-/* Windows
-** Prohibit:  all control characters, including tab, \r and \n.
-** Escape:    (space) " # $ % & ' ( ) * ; < > ? [ ] ^ ` { | }
-*/
-/*  x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf  */
-     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x */
-     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 1x */
-     1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0, /* 2x */
-     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  1,  1, /* 3x */
-     1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 4x */
-     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  1,  1,  0, /* 5x */
-     1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 6x */
-     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  1, /* 7x */
-#else
-/* Unix
+/*
 ** Prohibit:  all control characters, including tab, \r and \n
 ** Escape:    (space) ! " # $ % & ' ( ) * ; < > ? [ \ ] ^ ` { | }
 */
@@ -410,7 +395,6 @@ static const char aSafeChar[256] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0, /* 5x */
      1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 6x */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  1, /* 7x */
-#endif
     /* all bytes 0x80 through 0xbf are unescaped, being secondary
     ** bytes to UTF8 characters.  Bytes 0xc0 through 0xff are the
     ** first byte of a UTF8 character and do get escaped */
@@ -424,75 +408,88 @@ static const char aSafeChar[256] = {
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5  /* fx */
 };
 
+/*
+** Return true if the input contains special characters that need
+** to be escaped.
+*/
+int need_escape(const char *zIn){
+  size_t i;
+  unsigned char c;
+
+  /* Look for characters that need to be escaped */
+  for(i=0; (c = (unsigned char)zIn[i])!=0; i++){
+    if( aSafeChar[c] ) return 1;
+  }
+  return 0;
+}
+
+/*
+** Increment this global variable whenever the isFilename==2 hex encoding
+** is used.
+*/
+static int nHexEncode = 0;
+
 /*
 ** pStr is a shell command under construction.  This routine safely
-** appends filename argument zIn.  It returns 0 on success or non-zero
-** on any error.
+** appends filename argument zIn.
 **
 ** The argument is escaped if it contains white space or other characters
 ** that need to be escaped for the shell.  If zIn contains characters
 ** that cannot be safely escaped, then throw a fatal error.
 **
-** If the isFilename argument is true, then the argument is expected
-** to be a filename.  As shell commands commonly have command-line
+** If the isFilename argument is 1 or 2, then the argument is expected
+** to be a filename.  isFilename==1 means to use shell-style escaping
+** as the argument is going directly to the shell.  isFilename==2 means
+** to escape as hexadecimal (using A-P as the hex digits) instead of
+** shell escaping.  isFilename==2 argument are interpreted by another
+** instance of sqlite3_rsync which knows how to decode them.
+**
+** For isFilename==1, because shell commands commonly have command-line
 ** options that begin with "-" and since we do not want an attacker
 ** to be able to invoke these switches using filenames that begin
-** with "-", if zIn begins with "-", prepend an additional "./"
-** (or ".\\" on Windows).
+** with "-", if zIn begins with "-", prepend an additional "./".
+**
+** For isFilename==2, if the filename could be mistaken for a hexadecimal
+** string (if it is an even number of characters and all characters are
+** between A and P) then "./" is prepended so that the receiver
+** will know that it is not an encoded filename.
 */
-int append_escaped_arg(sqlite3_str *pStr, const char *zIn, int isFilename){
-  int i;
-  unsigned char c;
-  int needEscape = 0
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_fdopen`
- 外部函数：`_open_osfhandle`
- 外部函数：`a00`
- 外部函数：`a01`
- 外部函数：`a02`
- 外部函数：`a03`
- 外部函数：`a04`
- 外部函数：`a10`
- 外部函数：`a11`
- 外部函数：`a12`
- 外部函数：`a13`
- 外部函数：`a14`
- 外部函数：`a20`
- 外部函数：`a21`
- 外部函数：`a22`
- 外部函数：`a23`
- 外部函数：`a24`
- 外部函数：`a30`
- 外部函数：`a31`
- 外部函数：`a32`
- 外部函数：`a33`
- 外部函数：`a34`
- 外部函数：`a40`
- 外部函数：`a41`
- 外部函数：`a42`
- 外部函数：`a43`
- 外部函数：`a44`
- 外部函数：`agghash`
- 外部函数：`c`
- 外部函数：`close`
- 外部函数：`defined`
- 外部函数：`dup`
- 外部函数：`execl`
- 外部函数：`exit`
- 外部函数：`failed`
- 外部函数：`fclose`
- 外部函数：`fdopen`
- 外部函数：`fflush`
- 外部函数：`fgetc`
- 外部函数：`fopen`
- 外部函数：`fork`
- 外部函数：`fprintf`
- 外部函数：`fputc`
- 外部函数：`fread`
- 外部函数：`fwrite`
- 外部函数：`hash`
- 外部函数：`isalpha`
- 外部函数：`isspace`
- 外部函数：`malloc`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`pipe`
- 外部函数：`popen`
- 外部函数：`printf`
- 外部函数：`putc`
- 外部函数：`sendHash`
- 外部函数：`signal`
- 外部函数：`sqlite3_aggregate_context`
- 外部函数：`sqlite3_bind_blob`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_close`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_create_function`
- 外部函数：`sqlite3_db_config`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_next_stmt`
- 外部函数：`sqlite3_open`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_result_blob`
- 外部函数：`sqlite3_sourceid`
- 外部函数：`sqlite3_sql`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_value`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vfs_find`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sqlite_dbpage`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strstr`
- 外部函数：`u`
- 外部函数：`unknown`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vfprintf`
- 外部函数：`xCurrentTimeInt64`
- 大写宏：`ALL`
- 大写宏：`ASCII`
- 大写宏：`ATTACH`
- 大写宏：`BEGIN`
- 大写宏：`BOOL`
- 大写宏：`COMMIT`
- 大写宏：`CP_UTF8`
- 大写宏：`CREATE`
- 大写宏：`CROSS`
- 大写宏：`DELETE`
- 大写宏：`DWORD`
- 大写宏：`EOF`
- 大写宏：`ERROR`
- 大写宏：`FALSE`
- 大写宏：`FILE`
- 大写宏：`FROM`
- 大写宏：`HANDLE`
- 大写宏：`HANDLE_FLAG_INHERIT`
- 大写宏：`IMMEDIATE`
- 大写宏：`INSERT`
- 大写宏：`INT`
- 大写宏：`INTEGER`
- 大写宏：`INTO`
- 大写宏：`JOIN`
- 大写宏：`KEY`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`PATH`
- 大写宏：`PID`
- 大写宏：`PRAGMA`
- 大写宏：`PRIMARY`
- 大写宏：`PROCESS_INFORMATION`
- 大写宏：`RECURSIVE`
- 大写宏：`REPLACE`
- 大写宏：`REPLICA`
- 大写宏：`ROLLBACK`
- 大写宏：`SECURITY_ATTRIBUTES`
- 大写宏：`SELECT`
- 大写宏：`SHA3`
- 大写宏：`SIGPIPE`
- 大写宏：`SIG_IGN`
- 大写宏：`SQL`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_DBCONFIG_WRITABLE_SCHEMA`
- 大写宏：`SQLITE_DETERMINISTIC`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_INNOCUOUS`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`SSH`
- 大写宏：`STARTF_USESTDHANDLES`
- 大写宏：`STARTUPINFOW`
- 大写宏：`STD_ERROR_HANDLE`
- 大写宏：`TABLE`
- 大写宏：`TRUE`
- 大写宏：`UNION`
- 大写宏：`UTF`
- 大写宏：`UTF8`
- 大写宏：`VALUES`
- 大写宏：`WAL`
- 大写宏：`WHERE`
- 大写宏：`WITH`
- 外部类型：`Add`
- 外部类型：`All`
- 外部类型：`Any`
- 外部类型：`Application`
- 外部类型：`As`
- 外部类型：`Attach`
- 外部类型：`Bad`
- 外部类型：`Big`
- 外部类型：`Byte`
- 外部类型：`Bytes`
- 外部类型：`Check`
- 外部类型：`Child`
- 外部类型：`Command`
- 外部类型：`Create`
- 外部类型：`Current`
- 外部类型：`Dispatch`
- 外部类型：`Do`
- 外部类型：`End`
- 外部类型：`Environment`
- 外部类型：`Error`
- 外部类型：`Escape`
- 外部类型：`First`
- 外部类型：`Fossil`
- 外部类型：`Handles`
- 外部类型：`Hash`
- 外部类型：`HashContext`
- 外部类型：`Hash_BYTEORDER`
- 外部类型：`How`
- 外部类型：`If`
- 外部类型：`Info`
- 外部类型：`Informational`
- 外部类型：`Inherit`
- 外部类型：`Initial`
- 外部类型：`It`
- 外部类型：`Journal`
- 外部类型：`Known`
- 外部类型：`Little`
- 外部类型：`Look`
- 外部类型：`Name`
- 外部类型：`New`
- 外部类型：`Next`
- 外部类型：`No`
- 外部类型：`Non`
- 外部类型：`Nothing`
- 外部类型：`Number`
- 外部类型：`One`
- 外部类型：`Oops`
- 外部类型：`Parse`
- 外部类型：`Process`
- 外部类型：`Prohibit`
- 外部类型：`Put`
- 外部类型：`Quoting`
- 外部类型：`Read`
- 外部类型：`Ready`
- 外部类型：`Replica`
- 外部类型：`Report`
- 外部类型：`Request`
- 外部类型：`Respond`
- 外部类型：`SQLiteRsync`
- 外部类型：`Separate`
- 外部类型：`Standard`
- 外部类型：`Startup`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Thread`
- 外部类型：`Time`
- 外部类型：`Transaction`
- 外部类型：`Unix`
- 外部类型：`Unknown`
- 外部类型：`Welcome`
- 外部类型：`Windows`
- 外部类型：`Write`
- 外部类型：`wchar_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e670b6f7d7` → 本草稿移入 `cases/defect/auto-sqlite-e670b6f7d7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
