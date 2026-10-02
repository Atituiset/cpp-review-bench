# auto-sqlite-6201195ec3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#9696acb0c77f1c1a7a400446686debc1312c94bc](https://github.com/sqlite/sqlite/commit/9696acb0c77f1c1a7a400446686debc1312c94bc) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 101 |
| 编译错误数（gcc syntax-only） | 43（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9696acb0c77f1c1a7a400446686debc1312c94bc (https://github.com/sqlite/sqlite/commit/9696acb0c77f1c1a7a400446686debc1312c94bc)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 5341；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9696acb0c77f1c1a7a400446686debc1312c94bc Change the way the session module applies changesets containing two or more UPDATE changes that form a dependency loop i :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2862,6 +2862,33 @@ static int sessionPrepare(
   return rc;
 }
 
+/*
+** Prepare the statement specified by printf format zFmt and its trailing
+** arguments. 
+*/
+static void sessionPrepareMprintf(
+  int *pRc,
+  sqlite3 *db, 
+  sqlite3_stmt **pp, 
+  char **pzErrmsg,
+  const char *zFmt,
+  ...
+){
+  if( *pRc==SQLITE_OK ){
+    char *zSql;
+    va_list ap;
+    va_start(ap, zFmt);
+    zSql = sqlite3_vmprintf(zFmt, ap);
+    if( zSql==0 ){
+      *pRc = SQLITE_NOMEM;
+    }else{
+      *pRc = sessionPrepare(db, pp, pzErrmsg, zSql);
+      sqlite3_free(zSql);
+    }
+    va_end(ap);
+  }
+}
+
 /*
 ** Formulate and prepare a SELECT statement to retrieve a row from table
 ** zTab in database zDb based on its primary key. i.e.
@@ -5265,161 +5292,215 @@ static int sessionApplyRetryBuffer(
 }
 
 /*
-** Check if table zTab in the "main" database of db is a WITHOUT ROWID
-** table. 
+** Buffer aUnique[] is pApply->nCol entries in size. This function sets
+** aUnique[i] to true if column i of table zTab in the "main" database
+** of db is part of at least one UNIQUE constraint, or to false otherwise.
+**
+** If the table has a unique index on an expression, or a partial unique
+** index, all entries of aUnique[] are set to true.
 **
-** If no error occurs, return SQLITE_OK and set output variable (*pbWR) to 
-** true if zTab is a WITHOUT ROWID table, or false otherwise. Or, if an
-** error does occur, return an SQLite error code. The final value of (*pbWR)
-** is undefined in this case.
+** SQLITE_OK is returned if successful, or an SQLite error code otherwise.
 */
-static int sessionTableIsWithoutRowid(sqlite3 *db, const char *zTab, int *pbWR){
-  sqlite3_stmt *pList = 0;
-  char *zSql = 0;
+static int sessionUniqueColumns(
+  sqlite3 *db,                    /* Database handle */
+  const char *zTab,               /* Table name */
+  SessionApplyCtx *pApply,        /* Apply context */
+  u8 *aUnique                     /* OUT: Array of pApply->nCol flags */
+){
+  sqlite3_stmt *pList = 0;        /* PRAGMA index_list */
   int rc = SQLITE_OK;
 
-  zSql = sqlite3_mprintf("PRAGMA table_list = %Q", zTab);
-  if( zSql==0 ){
-    rc = SQLITE_NOMEM;
-  }else{
-    rc = sqlite3_prepare_v2(db, zSql, -1, &pList, 0);
-    sqlite3_free(zSql);
+  /* Ordinary PRAGMA statements are used here instead of the equivalent
+  ** table-valued functions (pragma_index_list() etc.) so that this works
+  ** in builds with SQLITE_OMIT_VIRTUALTABLE defined.  */
+  memset(aUnique, 0, pApply->nCol);
+  sessionPrepareMprintf(&rc, db, &pList, &pApply->zErr,
+      "PRAGMA main.index_list(%Q)", zTab
+  );
+  while( rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pList) ){
+    /* Columns of PRAGMA index_list are (seq, name, unique, origin, partial) */
+    const char *zIdx = (const char*)sqlite3_column_text(pList, 1);
+    int bUnique = sqlite3_column_int(pList, 2);
+    int bPartial = sqlite3_column_int(pList, 4);
+    sqlite3_stmt *pInfo = 0;      /* PRAGMA index_xinfo */
+
+    if( bUnique==0 ) continue;
+    sessionPrepareMprintf(&rc, db, &pInfo, &pApply->zErr,
+        "PRAGMA main.index_xinfo(%Q)", zIdx
+    );
+    while( rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pInfo) ){
+      /* Columns of PRAGMA index_xinfo are (seqno, cid, name, desc, coll, key)*/
+      int iCid = sqlite3_column_int(pInfo, 1);
+      const char *zCol = (const char*)sqlite3_column_text(pInfo, 2);
+      int bKey = sqlite3_column_int(pInfo, 5);
+      int ii;
+      if( bKey==0 ) continue;
+      for(ii=0; ii<pApply->nCol; ii++){
+        if( bPartial
+         || iCid==-2
+         || (zCol && 0==sqlite3_stricmp(zCol, pApply->azCol[ii]))
+        ){
+          aUnique[ii] = 1;
+        }
+      }
+    }
+    if( rc==SQLITE_OK ){
+      rc = sqlite3_finalize(pInfo);
+    }else{
+      sqlite3_finalize(pInfo);
+    }
   }
-
   if( rc==SQLITE_OK ){
-    sqlite3_step(pList);
-    *pbWR = sqlite3_column_int(pList, 4);
     rc = sqlite3_finalize(pList);
+  }else{
+    sqlite3_finalize(pLis
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`and`
- 外部函数：`assert`
- 外部函数：`in`
- 外部函数：`int`
- 外部函数：`memcpy`
- 外部函数：`of`
- 外部函数：`sessionTableIsWithoutRowid`
- 外部函数：`sessionUpdateToDeleteInsert`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3ValueFree`
- 外部函数：`sqlite3_bind_int`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_value`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_value`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`the`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`value`
- 外部函数：`xValue`
- 大写宏：`COVERAGE`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`NOMEM`
- 大写宏：`NULL`
- 大写宏：`OOM`
- 大写宏：`OUT`
- 大写宏：`PRAGMA`
- 大写宏：`RELEASE`
- 大写宏：`ROLLBACK`
- 大写宏：`ROWID`
- 大写宏：`SAVEPOINT`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_CONSTRAINT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_DELETE`
- 大写宏：`SQLITE_INSERT`
- 大写宏：`SQLITE_MISUSE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RANGE`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_UPDATE`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 大写宏：`WITHOUT`
- 外部类型：`Allocations`
- 外部类型：`Apply`
- 外部类型：`Attempt`
- 外部类型：`Bind`
- 外部类型：`Buffer`
- 外部类型：`Changeset`
- 外部类型：`Check`
- 外部类型：`Current`
- 外部类型：`Database`
- 外部类型：`Delete`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`Formulate`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Input`
- 外部类型：`It`
- 外部类型：`Iterator`
- 外部类型：`Make`
- 外部类型：`Neither`
- 外部类型：`New`
- 外部类型：`Number`
- 外部类型：`Old`
- 外部类型：`Once`
- 外部类型：`Or`
- 外部类型：`Parameter`
- 外部类型：`Points`
- 外部类型：`Primary`
- 外部类型：`Return`
- 外部类型：`SQLite`
- 外部类型：`SessionApplyCtx`
- 外部类型：`SessionBuffer`
- 外部类型：`SessionInput`
- 外部类型：`Skip`
- 外部类型：`Statement`
- 外部类型：`String`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Used`
- 外部类型：`Value`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6201195ec3` → 本草稿移入 `cases/defect/auto-sqlite-6201195ec3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
