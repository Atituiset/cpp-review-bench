# auto-sqlite-0c66f24120

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d3d85df39e5ac1314974fb0b48389235b93e9e14](https://github.com/sqlite/sqlite/commit/d3d85df39e5ac1314974fb0b48389235b93e9e14) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d3d85df39e5ac1314974fb0b48389235b93e9e14 (https://github.com/sqlite/sqlite/commit/d3d85df39e5ac1314974fb0b48389235b93e9e14)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 224；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d3d85df39e5ac1314974fb0b48389235b93e9e14 Enhance the readfile() function in the fileio.c extension so that it :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -221,8 +221,10 @@ static int fileio_mkdir(const char *zPath){
 */
 static void readFileContents(sqlite3_context *ctx, const char *zName){
   FILE *in;
-  sqlite3_int64 nIn;
-  void *pBuf;
+  sqlite3_int64 nIn;              /* Size of the file according to ftell() */
+  sqlite3_int64 nAlloc;           /* Bytes allocated for pBuf */
+  sqlite3_int64 nRead = 0;        /* Bytes read so far */
+  unsigned char *pBuf;
   sqlite3 *db;
   int mxBlob;
 
@@ -231,27 +233,54 @@ static void readFileContents(sqlite3_context *ctx, const char *zName){
     /* File does not exist or is unreadable. Leave the result set to NULL. */
     return;
   }
+  /* The size reported by ftell() is only a hint.  It is -1 for pipes and
+  ** FIFOs and 0 for many files under /proc.  Keep reading until EOF. */
   fseek(in, 0, SEEK_END);
   nIn = ftell(in);
   rewind(in);
+  if( nIn<0 ) nIn = 0;
   db = sqlite3_context_db_handle(ctx);
   mxBlob = sqlite3_limit(db, SQLITE_LIMIT_LENGTH, -1);
   if( nIn>mxBlob ){
     sqlite3_result_error_code(ctx, SQLITE_TOOBIG);
     fclose(in);
     return;
   }
-  pBuf = sqlite3_malloc64( nIn ? nIn : 1 );
+  /* One extra byte so that EOF on a regular file is seen without a
+  ** reallocation. */
+  nAlloc = nIn+1;
+  pBuf = sqlite3_malloc64( nAlloc );
   if( pBuf==0 ){
     sqlite3_result_error_nomem(ctx);
     fclose(in);
     return;
   }
-  if( nIn==(sqlite3_int64)fread(pBuf, 1, (size_t)nIn, in) ){
-    sqlite3_result_blob64(ctx, pBuf, nIn, sqlite3_free);
-  }else{
+  while( 1 ){
+    unsigned char *pNew;
+    nRead += fread(pBuf+nRead, 1, (size_t)(nAlloc-nRead), in);
+    if( nRead>mxBlob ){
+      sqlite3_result_error_code(ctx, SQLITE_TOOBIG);
+      sqlite3_free(pBuf);
+      fclose(in);
+      return;
+    }
+    if( nRead<nAlloc ) break;      /* Short read means EOF or an error */
+    nAlloc *= 2;
+    if( nAlloc>(sqlite3_int64)mxBlob+1 ) nAlloc = (sqlite3_int64)mxBlob+1;
+    pNew = sqlite3_realloc64(pBuf, nAlloc);
+    if( pNew==0 ){
+      sqlite3_result_error_nomem(ctx);
+      sqlite3_free(pBuf);
+      fclose(in);
+      return;
+    }
+    pBuf = pNew;
+  }
+  if( ferror(in) ){
     sqlite3_result_error_code(ctx, SQLITE_IOERR);
     sqlite3_free(pBuf);
+  }else{
+    sqlite3_result_blob64(ctx, pBuf, nRead, sqlite3_free);
   }
   fclose(in);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fclose`
- 外部函数：`fread`
- 外部函数：`fseek`
- 外部函数：`ftell`
- 外部函数：`rewind`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_limit`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_result_blob64`
- 外部函数：`sqlite3_result_error_code`
- 外部函数：`sqlite3_result_error_nomem`
- 大写宏：`FILE`
- 大写宏：`NULL`
- 大写宏：`SEEK_END`
- 大写宏：`SQLITE_IOERR`
- 大写宏：`SQLITE_LIMIT_LENGTH`
- 大写宏：`SQLITE_TOOBIG`
- 外部类型：`File`
- 外部类型：`Leave`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-0c66f24120` → 本草稿移入 `cases/defect/auto-sqlite-0c66f24120/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
