# auto-sqlite-b5e674c22c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#076ab6c28636bb73ff84c41d3ddbb59c78d46a7c](https://github.com/sqlite/sqlite/commit/076ab6c28636bb73ff84c41d3ddbb59c78d46a7c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #076ab6c28636bb73ff84c41d3ddbb59c78d46a7c (https://github.com/sqlite/sqlite/commit/076ab6c28636bb73ff84c41d3ddbb59c78d46a7c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 076ab6c28636bb73ff84c41d3ddbb59c78d46a7c Prevent application-defined functions that invoke DDL statement or :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3221,6 +3221,22 @@ static void checkActiveVdbeCnt(sqlite3 *db){
 #define checkActiveVdbeCnt(x)
 #endif
 
+/*
+** Return a bitmask of all schemas referenced by active VDBEs, other
+** than pExcept.
+*/
+void sqlite3VdbeSchemaInUse(sqlite3 *db, Vdbe *pExcept, yDbMask *pM){
+  Vdbe *p;
+  yDbMask m;
+  DbMaskZero(m);
+  for(p=db->pVdbe; p; p=p->pVNext){
+    if( p==pExcept ) continue;
+    if( p->eVdbeState!=VDBE_RUN_STATE ) continue;
+    DbMaskUnion(m,p->btreeMask);
+  }
+  memcpy(pM, &m, sizeof(m));
+}
+
 /*
 ** If the Vdbe passed as the first argument opened a statement-transaction,
 ** close it now. Argument eOp must be either SAVEPOINT_ROLLBACK or
@@ -3527,6 +3543,13 @@ int sqlite3VdbeHalt(Vdbe *p){
     sqlite3ConnectionUnlocked(db);
   }
 
+  /* Do any pending schema resets that were deferred because multiple
+  ** VDBEs where active at the same time.
+  */
+  if( db->bPendingSchemaReset && db->nVdbeActive==0 ){
+    sqlite3ResetOneSchema(db, -1, 0);
+  }
+
   assert( db->nVdbeActive>0 || db->autoCommit==0 || db->nStatement==0 );
   return (p->rc==SQLITE_BUSY ? SQLITE_BUSY : SQLITE_OK);
 }
@@ -5294,6 +5317,23 @@ void sqlite3ExpirePreparedStatements(sqlite3 *db, int iCode){
   }
 }
 
+/*
+** Expire (urgently, with iCode 0) every prepared statement that is using
+** a particular schema (iDb).
+*/
+void sqlite3ExpireStmtsUsingSchema(
+  sqlite3 *db,      /* The database connection statements belong to */
+  int iDb,          /* Index of the schema they are using */
+  Vdbe *pExcept     /* Except, don't expire this one */
+){
+  Vdbe *p;
+  for(p = db->pVdbe; p; p=p->pVNext){
+    if( p==pExcept ) continue;
+    if( !DbMaskTest(p->btreeMask, iDb) ) continue;
+    p->expired = 1;
+  }
+}
+
 /*
 ** Return the database associated with the Vdbe.
 */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`checkActiveVdbeCnt`
- 外部函数：`sqlite3ConnectionUnlocked`
- 大写宏：`SAVEPOINT_ROLLBACK`
- 大写宏：`SQLITE_BUSY`
- 大写宏：`SQLITE_OK`
- 外部类型：`Argument`
- 外部类型：`If`
- 外部类型：`Return`
- 外部类型：`Vdbe`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b5e674c22c` → 本草稿移入 `cases/defect/auto-sqlite-b5e674c22c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
