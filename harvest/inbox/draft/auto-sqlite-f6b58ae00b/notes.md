# auto-sqlite-f6b58ae00b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b5fb535663df6593c749cb48d1f5d5672988d959](https://github.com/sqlite/sqlite/commit/b5fb535663df6593c749cb48d1f5d5672988d959) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b5fb535663df6593c749cb48d1f5d5672988d959 (https://github.com/sqlite/sqlite/commit/b5fb535663df6593c749cb48d1f5d5672988d959)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 218；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b5fb535663df6593c749cb48d1f5d5672988d959 Improve error propagation in the completion extension. [bugs:238acde3c8|Bug report 2026-11-25T00:55:42Z]. :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -181,6 +181,18 @@ static int completionClose(sqlite3_vtab_cursor *cur){
   return SQLITE_OK;
 }
 
+/*
+** Propagate the most recent error message from the database
+** connection into the virtual table. Returns rc unless copying
+** of the error fails, in which case it returns SQLITE_NOMEM.
+*/
+static int completionError(completion_cursor *pCur, int rc){
+  sqlite3_vtab *pVtab = pCur->base.pVtab;
+  sqlite3_free(pVtab->zErrMsg);
+  pVtab->zErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(pCur->db));
+  return pVtab->zErrMsg ? rc : SQLITE_NOMEM;
+}
+
 /*
 ** Advance a completion_cursor to its next row of output.
 **
@@ -215,8 +227,9 @@ static int completionNext(sqlite3_vtab_cursor *cur){
       }
       case COMPLETION_DATABASES: {
         if( pCur->pStmt==0 ){
-          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1,
-                             &pCur->pStmt, 0);
+          rc = sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1,
+                                  &pCur->pStmt, 0);
+          if( rc ) return completionError(pCur, rc);
         }
         iCol = 1;
         eNextPhase = COMPLETION_TABLES;
@@ -228,7 +241,12 @@ static int completionNext(sqlite3_vtab_cursor *cur){
           sqlite3_str* pStr = sqlite3_str_new(pCur->db);
           char *zSql = 0;
           const char *zSep = "";
-          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1, &pS2, 0);
+          rc = sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1,
+                                  &pS2, 0);
+          if( rc ){
+            sqlite3_free(sqlite3_str_finish(pStr));
+            return completionError(pCur, rc);
+          }
           while( sqlite3_step(pS2)==SQLITE_ROW ){
             const char *zDb = (const char*)sqlite3_column_text(pS2, 1);
             sqlite3_str_appendf(pStr,
@@ -242,10 +260,10 @@ static int completionNext(sqlite3_vtab_cursor *cur){
           zSql = sqlite3_str_finish(pStr);
           if( zSql==0 ) return SQLITE_NOMEM;
           if( rc==SQLITE_OK ){
-            sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
+            rc = sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
           }
           sqlite3_free(zSql);
-          if( rc ) return rc;
+          if( rc ) return completionError(pCur, rc);
         }
         iCol = 0;
         eNextPhase = COMPLETION_COLUMNS;
@@ -257,7 +275,12 @@ static int completionNext(sqlite3_vtab_cursor *cur){
           sqlite3_str *pStr = sqlite3_str_new(pCur->db);
           char *zSql = 0;
           const char *zSep = "";
-          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1, &pS2, 0);
+          rc = sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1,
+                                  &pS2, 0);
+          if( rc ){
+            sqlite3_free(sqlite3_str_finish(pStr));
+            return completionError(pCur, rc);
+          }
           while( sqlite3_step(pS2)==SQLITE_ROW ){
             const char *zDb = (const char*)sqlite3_column_text(pS2, 1);
             sqlite3_str_appendf(pStr,
@@ -273,10 +296,10 @@ static int completionNext(sqlite3_vtab_cursor *cur){
           zSql = sqlite3_str_finish(pStr);
           if( zSql==0 ) return SQLITE_NOMEM;
           if( rc==SQLITE_OK ){
-            sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
+            rc = sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
           }
           sqlite3_free(zSql);
-          if( rc ) return rc;
+          if( rc ) return completionError(pCur, rc);
         }
         iCol = 0;
         eNextPhase = COMPLETION_EOF;
@@ -296,7 +319,7 @@ static int completionNext(sqlite3_vtab_cursor *cur){
         rc = sqlite3_finalize(pCur->pStmt);
         pCur->pStmt = 0;
         pCur->ePhase = eNextPhase;
-        if( rc ) return rc;
+        if( rc ) return completionError(pCur, rc);
         continue;
       }
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_new`
- 大写宏：`PRAGMA`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 外部类型：`Advance`
- 外部类型：`Also`
- 外部类型：`TRIGGERs`
- 外部类型：`VIEWs`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f6b58ae00b` → 本草稿移入 `cases/defect/auto-sqlite-f6b58ae00b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
