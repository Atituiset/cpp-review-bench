# auto-sqlite-d072ca9be4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1e946f434429ebf27c5c567876cb47eee29dc5a6](https://github.com/sqlite/sqlite/commit/1e946f434429ebf27c5c567876cb47eee29dc5a6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1e946f434429ebf27c5c567876cb47eee29dc5a6 (https://github.com/sqlite/sqlite/commit/1e946f434429ebf27c5c567876cb47eee29dc5a6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1e946f434429ebf27c5c567876cb47eee29dc5a6 Fix RBU so that it works with ANY columns in STRICT tables. Report [bugs:/info/2026-10-18T20:39:51Z | 2026-10-18T20:39:5 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1347,6 +1347,25 @@ static void rbuObjIterCacheIndexedCols(sqlite3rbu *p, RbuObjIter *pIter){
   if( bIndex==0 ) pIter->abIndexed = 0;
 }
 
+/*
+** Return true if table zTbl in the main database of the target is a
+** STRICT table, or false otherwise. If an error occurs, an error code
+** and message are left in the RBU handle and zero is returned.
+*/
+static int rbuTableIsStrict(sqlite3rbu *p, const char *zTbl){
+  int bStrict = 0;
+  sqlite3_stmt *pStmt = 0;
+  if( p->rc==SQLITE_OK ){
+    p->rc = prepareFreeAndCollectError(p->dbMain, &pStmt, &p->zErrmsg,
+        sqlite3_mprintf("PRAGMA main.table_list(%Q)", zTbl)
+    );
+    if( p->rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pStmt) ){
+      bStrict = sqlite3_column_int(pStmt, 5);
+    }
+    rbuFinalize(p, pStmt);
+  }
+  return bStrict;
+}
 
 /*
 ** If they are not already populated, populate the pIter->azTblCol[],
@@ -1365,6 +1384,7 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
     int bRbuRowid = 0;            /* If input table has column "rbu_rowid" */
     int iOrder = 0;
     int iTnum = 0;
+    int bStrict = 0;              /* True if target is a STRICT table */
 
     /* Figure out the type of table this step will deal with. */
     assert( pIter->eType==0 );
@@ -1416,6 +1436,8 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
       );
     }
 
+    bStrict = rbuTableIsStrict(p, pIter->zTbl);
+
     /* Check that all non-HIDDEN columns in the destination table are also
     ** present in the input table. Populate the abTblPk[], azTblType[] and
     ** aiTblOrder[] arrays at the same time.
@@ -1457,6 +1479,9 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
           SWAP(char*, pIter->azTblCol[i], pIter->azTblCol[iOrder]);
         }
 
+        /* In a STRICT table, ANY means no affinity. In the non-STRICT
+        ** imposter it would mean NUMERIC, so declare it without a type. */
+        if( bStrict && sqlite3_stricmp(zType, "ANY")==0 ) zType = "";
         pIter->azTblType[iOrder] = rbuStrndup(zType, &p->rc);
         assert( iPk>=0 );
         pIter->abTblPk[iOrder] = (u8)iPk;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`strlen`
- 大写宏：`HIDDEN`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`TYPE`
- 外部类型：`Check`
- 外部类型：`Figure`
- 外部类型：`If`
- 外部类型：`Populate`
- 外部类型：`size_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-d072ca9be4` → 本草稿移入 `cases/defect/auto-sqlite-d072ca9be4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
