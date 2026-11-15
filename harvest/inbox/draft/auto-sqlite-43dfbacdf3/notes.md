# auto-sqlite-43dfbacdf3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b5918a2af3db467924224506dcf51b801954a9a8](https://github.com/sqlite/sqlite/commit/b5918a2af3db467924224506dcf51b801954a9a8) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 92 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b5918a2af3db467924224506dcf51b801954a9a8 (https://github.com/sqlite/sqlite/commit/b5918a2af3db467924224506dcf51b801954a9a8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 516；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b5918a2af3db467924224506dcf51b801954a9a8 Improved shell escaping on "ssh" commands run by sqlite3_rsync.  Note that "~" :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -376,7 +376,7 @@ static int pclose2(FILE *pIn, FILE *pOut, int childPid){
 **
 **    0      Ordinary character.  No encoding required
 **    1      Needs to be escaped
-**    2      Illegal character.  Do not allow in a filename
+**    2      Control characters or bytes of a UTF-8 other than the first
 **    3      First byte of a 2-byte UTF-8
 **    4      First byte of a 3-byte UTF-8
 **    5      First byte of a 4-byte UTF-8
@@ -390,11 +390,11 @@ static const char aSafeChar[256] = {
      2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x */
      2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 1x */
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0, /* 2x */
-     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  1,  1, /* 3x */
+     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1, /* 3x */
      1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 4x */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0, /* 5x */
      1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 6x */
-     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  1, /* 7x */
+     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1, /* 7x */
     /* all bytes 0x80 through 0xbf are unescaped, being secondary
     ** bytes to UTF8 characters.  Bytes 0xc0 through 0xff are the
     ** first byte of a UTF8 character and do get escaped */
@@ -410,7 +410,10 @@ static const char aSafeChar[256] = {
 
 /*
 ** Return true if the input contains special characters that need
-** to be escaped.
+** to be escaped.  Actually, just to be conservative, this routine
+** returns true if there is anything in the string that it suspicious,
+** including Unicode characters.  So technically it might return
+** a false-positive.  It should not return a false-negative, however.
 */
 int need_escape(const char *zIn){
   size_t i;
@@ -434,8 +437,7 @@ static int nHexEncode = 0;
 ** appends filename argument zIn.
 **
 ** The argument is escaped if it contains white space or other characters
-** that need to be escaped for the shell.  If zIn contains characters
-** that cannot be safely escaped, then throw a fatal error.
+** that need to be escaped for the shell.
 **
 ** If the isFilename argument is 1 or 2, then the argument is expected
 ** to be a filename.  isFilename==1 means to use shell-style escaping
@@ -460,7 +462,7 @@ void append_escaped_arg(sqlite3_str *pStr, const char *zIn, int isFilename){
   size_t nIn = strlen(zIn);
 
   /* Separate from the previous argument by a space */
-  if( n>0 && !isspace(z[n-1]) ){
+  if( n>0 && !isspace((unsigned char)z[n-1]) ){
     sqlite3_str_appendchar(pStr, 1, ' ');
   }
 
@@ -506,32 +508,29 @@ void append_escaped_arg(sqlite3_str *pStr, const char *zIn, int isFilename){
     sqlite3_str_appendchar(pStr, 1, '"');
 #else
     /* Quoting strategy for unix:
-    ** If the name does not contain ', then surround the whole thing
-    ** with '...'.   If there is one or more ' characters within the
-    ** name, then put \ before each special character.
+    ** Surround the whole argument using '...' and escape internal '
+    ** characters as '\''
     */
-    if( strchr(zIn,'\'') ){
-      unsigned char c;
-      size_t i;
-      if( isFilename && zIn[0]=='-' ){
-        sqlite3_str_appendchar(pStr, 1, '.');
-        sqlite3_str_appendchar(pStr, 1, '/');
-      }
-      for(i=0; (c = (unsigned char)zIn[i])!=0; i++){
-        if( aSafeChar[c] && aSafeChar[c]!=2 ){
-          sqlite3_str_appendchar(pStr, 1, '\\');
-        }
-        sqlite3_str_appendchar(pStr, 1, (char)c);
+    sqlite3_str_appendchar(pStr, 1, '\'');
+    if( isFilename && zIn[0]=='-' ){
+      sqlite3_str_appendchar(pStr, 1, '.');
+      sqlite3_str_appendchar(pStr, 1, '/');
+    }
+    while( zIn[0] ){
+      char *zEsc = strchr(zIn, '\'');
+      size_t nn;
+      if( zEsc==0 ){
+        sqlite3_str_appendall(pStr, zIn);
+        break;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_fdopen`
- 外部函数：`_open_osfhandle`
- 外部函数：`close`
- 外部函数：`defined`
- 外部函数：`dup`
- 外部函数：`execl`
- 外部函数：`exit`
- 外部函数：`fdopen`
- 外部函数：`fork`
- 外部函数：`fprintf`
- 外部函数：`isspace`
- 外部函数：`launches`
- 外部函数：`malloc`
- 外部函数：`memset`
- 外部函数：`or`
- 外部函数：`pipe`
- 外部函数：`popen`
- 外部函数：`printf`
- 外部函数：`signal`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_str_value`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strspn`
- 外部函数：`types`
- 大写宏：`ABCDEFGHIJKLMNOP`
- 大写宏：`BOOL`
- 大写宏：`CP_UTF8`
- 大写宏：`DEBUG`
- 大写宏：`DWORD`
- 大写宏：`FALSE`
- 大写宏：`FILE`
- 大写宏：`FILENAME1`
- 大写宏：`FILENAME2`
- 大写宏：`HANDLE`
- 大写宏：`HANDLE_FLAG_INHERIT`
- 大写宏：`HOME`
- 大写宏：`HOST`
- 大写宏：`NULL`
- 大写宏：`ONLY`
- 大写宏：`OUT`
- 大写宏：`PATH`
- 大写宏：`PID`
- 大写宏：`PROCESS_INFORMATION`
- 大写宏：`SECURITY_ATTRIBUTES`
- 大写宏：`SIGPIPE`
- 大写宏：`SIG_IGN`
- 大写宏：`SSH`
- 大写宏：`STARTF_USESTDHANDLES`
- 大写宏：`STARTUPINFOW`
- 大写宏：`STD_ERROR_HANDLE`
- 大写宏：`TRUE`
- 大写宏：`USER`
- 大写宏：`UTF`
- 大写宏：`UTF8`
- 外部类型：`Any`
- 外部类型：`Application`
- 外部类型：`Bytes`
- 外部类型：`Check`
- 外部类型：`Child`
- 外部类型：`Command`
- 外部类型：`Could`
- 外部类型：`Create`
- 外部类型：`Current`
- 外部类型：`Do`
- 外部类型：`Encode`
- 外部类型：`Environment`
- 外部类型：`Error`
- 外部类型：`First`
- 外部类型：`Handles`
- 外部类型：`If`
- 外部类型：`Illegal`
- 外部类型：`Info`
- 外部类型：`Inherit`
- 外部类型：`Must`
- 外部类型：`Name`
- 外部类型：`Need`
- 外部类型：`Needs`
- 外部类型：`No`
- 外部类型：`Ordinary`
- 外部类型：`Process`
- 外部类型：`Put`
- 外部类型：`Quoting`
- 外部类型：`Read`
- 外部类型：`Return`
- 外部类型：`Separate`
- 外部类型：`Standard`
- 外部类型：`Startup`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Thread`
- 外部类型：`Use`
- 外部类型：`Write`
- 外部类型：`size_t`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-43dfbacdf3` → 本草稿移入 `cases/defect/auto-sqlite-43dfbacdf3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
