# auto-sqlite-5bfdb6703b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3](https://github.com/sqlite/sqlite/commit/e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 66 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3 (https://github.com/sqlite/sqlite/commit/e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 423；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3 Improved handling of empty filenames in the writefile() function that :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -412,6 +412,9 @@ static int makeDirectory(
 
   if( zCopy==0 ){
     rc = SQLITE_NOMEM;
+  }else if( zCopy[0]==0 ){
+    rc = SQLITE_ERROR;
+    sqlite3_free(zCopy);
   }else{
     int nCopy = (int)strlen(zCopy);
     int i = 1;
@@ -420,8 +423,8 @@ static int makeDirectory(
       STRUCT_STAT sStat;
       int rc2;
 
-      for(; zCopy[i]!='/' && i<nCopy; i++);
-      if( i==nCopy ) break;
+      for(; i<nCopy && zCopy[i]!='/'; i++);
+      if( i>=nCopy ) break;
       zCopy[i] = '\0';
 
       rc2 = fileStat(zCopy, &sStat);
@@ -592,7 +595,8 @@ static void writefileFunc(
 
   res = writeFile(context, zFile, argv[1], mode, mtime);
   if( res==1 && errno==ENOENT ){
-    if( makeDirectory(zFile)==SQLITE_OK ){
+    res = makeDirectory(zFile);
+    if( res==SQLITE_OK ){
       res = writeFile(context, zFile, argv[1], mode, mtime);
     }
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_wchmod`
- 外部函数：`_wmkdir`
- 外部函数：`_wstat`
- 外部函数：`fclose`
- 外部函数：`fwrite`
- 外部函数：`memset`
- 外部函数：`mkdir`
- 外部函数：`parameter`
- 外部函数：`sqlite3_fopen`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_result_int64`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`stat`
- 外部函数：`strlen`
- 外部函数：`symlink`
- 外部函数：`unlink`
- 外部函数：`winUtf8To16`
- 外部函数：`writefile`
- 大写宏：`BOOL`
- 大写宏：`CP_UTF8`
- 大写宏：`DWORD`
- 大写宏：`EEXIST`
- 大写宏：`FILE`
- 大写宏：`FILETIME`
- 大写宏：`FILE_FLAG_BACKUP_SEMANTICS`
- 大写宏：`FILE_WRITE_ATTRIBUTES`
- 大写宏：`HANDLE`
- 大写宏：`INVALID_HANDLE_VALUE`
- 大写宏：`LONGLONG`
- 大写宏：`LPFILETIME`
- 大写宏：`LPWSTR`
- 大写宏：`MODE`
- 大写宏：`MTIME`
- 大写宏：`NULL`
- 大写宏：`OPEN_EXISTING`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SYSTEMTIME`
- 大写宏：`S_ISDIR`
- 大写宏：`S_ISLNK`
- 大写宏：`ULARGE_INTEGER`
- 大写宏：`WCHAR`
- 大写宏：`WIN32`
- 大写宏：`WIN32_FIND_DATAW`
- 外部类型：`Context`
- 外部类型：`Data`
- 外部类型：`File`
- 外部类型：`HighPart`
- 外部类型：`LowPart`
- 外部类型：`QuadPart`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Windows`
- 外部类型：`mode_t`
- 外部类型：`stat`
- 外部类型：`time_t`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5bfdb6703b` → 本草稿移入 `cases/defect/auto-sqlite-5bfdb6703b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
