# auto-sqlite-8f6859928c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e5cb4b09573aeb53cbb28983be7234e42c48c07a](https://github.com/sqlite/sqlite/commit/e5cb4b09573aeb53cbb28983be7234e42c48c07a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 81 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e5cb4b09573aeb53cbb28983be7234e42c48c07a (https://github.com/sqlite/sqlite/commit/e5cb4b09573aeb53cbb28983be7234e42c48c07a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1490；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e5cb4b09573aeb53cbb28983be7234e42c48c07a Improve robustness of RBU against SQL injection attackes coming from :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1092,6 +1092,43 @@ static int rbuMPrintfExec(sqlite3rbu *p, sqlite3 *db, const char *zFmt, ...){
   return p->rc;
 }
 
+/*
+** Like rbuMPrintfExec() except that it is guaranteed to only run a
+** single statement.  If the generated SQL text includes two or more
+** statements, all but the first is ignored.
+**
+** If an error occurs, an error code and error message is stored in the
+** RBU handle. If an error has already occurred when this function is
+** called, it is a no-op.
+*/
+static int rbuMPrintfOneStmt(sqlite3rbu *p, sqlite3 *db, const char *zFmt, ...){
+  va_list ap;
+  char *zSql;
+  va_start(ap, zFmt);
+  zSql = sqlite3_vmprintf(zFmt, ap);
+  if( p->rc==SQLITE_OK ){
+    if( zSql==0 ){
+      p->rc = SQLITE_NOMEM;
+    }else{
+      sqlite3_stmt *pStmt;
+      p->rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
+      if( pStmt ){
+        if( p->rc==SQLITE_OK ){
+          while( sqlite3_step(pStmt)==SQLITE_ROW ){}
+        }
+        p->rc = sqlite3_finalize(pStmt);
+      }
+      if( p->rc!=SQLITE_OK ){
+        sqlite3_free(p->zErrmsg);
+        p->zErrmsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
+      }
+    }
+  }
+  sqlite3_free(zSql);
+  va_end(ap);
+  return p->rc;
+}
+
 /*
 ** Attempt to allocate and return a pointer to a zeroed block of nByte 
 ** bytes. 
@@ -1166,6 +1203,31 @@ static char *rbuStrndup(const char *zStr, int *pRc){
   return zRet;
 }
 
+/*
+** Return a copy of the declared column type zType that is safe to paste
+** into a CREATE TABLE statement.
+**
+** zType comes from the schema of a database that RBU does not control.
+** A type name may legally be a quoted string, so after the quotes are
+** removed (as PRAGMA table_xinfo does) it can contain any text at all,
+** including ")", ";" and CHECK or other column constraints. So it is
+** returned as a single double-quoted token, which the parser turns back
+** into the identical type name. An empty type is returned as an empty
+** string. The result must be freed using sqlite3_free().
+*/
+static char *rbuQuoteType(const char *zType, int *pRc){
+  char *zRet = 0;
+  if( *pRc==SQLITE_OK ){
+    if( zType==0 || zType[0]==0 ){
+      zRet = rbuStrndup("", pRc);
+    }else{
+      zRet = sqlite3_mprintf("\"%w\"", zType);
+      if( zRet==0 ) *pRc = SQLITE_NOMEM;
+    }
+  }
+  return zRet;
+}
+
 /*
 ** Finalize the statement passed as the second argument.
 **
@@ -1487,7 +1549,7 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
         /* In a STRICT table, ANY means no affinity. In the non-STRICT
         ** imposter it would mean NUMERIC, so declare it without a type. */
         if( bStrict && sqlite3_stricmp(zType, "ANY")==0 ) zType = "";
-        pIter->azTblType[iOrder] = rbuStrndup(zType, &p->rc);
+        pIter->azTblType[iOrder] = rbuQuoteType(zType, &p->rc);
         pIter->azTblDflt[iOrder] = rbuStrndup(zDflt, &p->rc);
         assert( iPk>=0 );
         pIter->abTblPk[iOrder] = (u8)iPk;
@@ -2161,7 +2223,7 @@ static void rbuCreateImposterTable2(sqlite3rbu *p, RbuObjIter *pIter){
     rbuFinalize(p, pXInfo);
 
     sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 1, tnum);
-    rbuMPrintfExec(p, p->dbMain,
+    rbuMPrintfOneStmt(p, p->dbMain,
         "CREATE TABLE rbu_imposter2(%z, PRIMARY KEY(%z)) WITHOUT ROWID", 
         zCols, zPk
     );
@@ -2228,7 +2290,7 @@ static void rbuCreateImposterTable(sqlite3rbu *p, RbuObjIter *pIter){
     }
 
     sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 1, tnum);
-    rbuMPrintfExec(p, p->dbMain, "CREATE TABLE \"rbu_imp_%w\"(%z)%s", 
+    rbuMPrintfOneStmt(p, p->dbMain, "CREATE TABLE \"rbu_imp_%w\"(%z)%s", 
         pIter->zTbl, zSql, 
         (pIter->eType==RBU_PK_WITHOUT_ROWID ? " WITHOUT ROWID" : "")
     );
@@ -2432,7 +2494,7 @@ static int rbuObjIterPrepareAll(
       /* Create the imposter table used to write to this index. */
       sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 0, 1);
       sqlit
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`db`
- 外部函数：`int`
- 外部函数：`memcpy`
- 外部函数：`rbuIsVacuum`
- 外部函数：`rbuMPrintfExec`
- 外部函数：`rbuSetupCheckpoint`
- 外部函数：`rbu_imposter2`
- 外部函数：`rbu_step`
- 外部函数：`rbu_tmp_insert`
- 外部函数：`sqlite3_db_handle`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_file_control`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_test_control`
- 外部函数：`stage`
- 外部函数：`state`
- 外部函数：`strlen`
- 大写宏：`AFTER`
- 大写宏：`ANY`
- 大写宏：`ATTACH`
- 大写宏：`BEGIN`
- 大写宏：`CREATE`
- 大写宏：`EXISTS`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`KEY`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`NUMERIC`
- 大写宏：`PRIMARY`
- 大写宏：`RBU`
- 大写宏：`RBU_STAGE_CKPT`
- 大写宏：`RBU_STATE_STAGE`
- 大写宏：`ROWID`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_FCNTL_RBU`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOTFOUND`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_TESTCTRL_IMPOSTER`
- 大写宏：`STRICT`
- 大写宏：`TABLE`
- 大写宏：`TEMP`
- 大写宏：`TRIGGER`
- 大写宏：`VFS`
- 大写宏：`WHERE`
- 大写宏：`WITHOUT`
- 外部类型：`Allocated`
- 外部类型：`Attempt`
- 外部类型：`Create`
- 外部类型：`Current`
- 外部类型：`Db`
- 外部类型：`Entries`
- 外部类型：`Error`
- 外部类型：`Fd`
- 外部类型：`File`
- 外部类型：`Finalize`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Iterator`
- 外部类型：`Largest`
- 外部类型：`Mark`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Pages`
- 外部类型：`Path`
- 外部类型：`RbuFrame`
- 外部类型：`RbuObjIter`
- 外部类型：`Rows`
- 外部类型：`See`
- 外部类型：`The`
- 外部类型：`Total`
- 外部类型：`Used`
- 外部类型：`Value`
- 外部类型：`rbu_file`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-8f6859928c` → 本草稿移入 `cases/defect/auto-sqlite-8f6859928c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
