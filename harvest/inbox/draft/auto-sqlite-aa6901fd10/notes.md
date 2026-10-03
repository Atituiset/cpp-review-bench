# auto-sqlite-aa6901fd10

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#61978d90eb246c93921ed7baa2ceaf4a035034a3](https://github.com/sqlite/sqlite/commit/61978d90eb246c93921ed7baa2ceaf4a035034a3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 95 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #61978d90eb246c93921ed7baa2ceaf4a035034a3 (https://github.com/sqlite/sqlite/commit/61978d90eb246c93921ed7baa2ceaf4a035034a3)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 146；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 61978d90eb246c93921ed7baa2ceaf4a035034a3 Do not allow surplus SQL text to come after the CREATE statement in the :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -129,6 +129,7 @@ int sqlite3InitCallback(void *pInit, int argc, char **argv, char **NotUsed){
     int rc;
     u8 saved_iDb = db->init.iDb;
     sqlite3_stmt *pStmt;
+    const char *zEnd;
     TESTONLY(int rcp);            /* Return code from sqlite3_prepare() */
 
     assert( db->init.busy );
@@ -143,7 +144,7 @@ int sqlite3InitCallback(void *pInit, int argc, char **argv, char **NotUsed){
     db->init.orphanTrigger = 0;
     db->init.azInit = (const char**)argv;
     pStmt = 0;
-    TESTONLY(rcp = ) sqlite3Prepare(db, argv[4], -1, 0, 0, &pStmt, 0);
+    TESTONLY(rcp = ) sqlite3Prepare(db, argv[4], -1, 0, 0, &pStmt, &zEnd);
     rc = db->errCode;
     assert( (rc&0xFF)==(rcp&0xFF) );
     db->init.iDb = saved_iDb;
@@ -159,6 +160,8 @@ int sqlite3InitCallback(void *pInit, int argc, char **argv, char **NotUsed){
           corruptSchema(pData, argv, sqlite3_errmsg(db));
         }
       }
+    }else if( zEnd[0] ){
+      corruptSchema(pData, argv, 0);
     }
     db->init.azInit = sqlite3StdType; /* Any array of string ptrs will do */
     sqlite3_finalize(pStmt);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`schema`
- 外部函数：`sqlite3BtreeBeginTrans`
- 外部函数：`sqlite3BtreeCommit`
- 外部函数：`sqlite3BtreeGetMeta`
- 外部函数：`sqlite3BtreeTxnState`
- 外部函数：`sqlite3DbNNFreeNN`
- 外部函数：`sqlite3ErrorMsg`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3OomFault`
- 外部函数：`sqlite3ParseObjectInit`
- 外部函数：`sqlite3ResetOneSchema`
- 外部函数：`sqlite3SchemaMutexHeld`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_prepare`
- 外部函数：`sqlite3_stmt_isexplain`
- 外部函数：`xCleanup`
- 大写宏：`BTREE_SCHEMA_VERSION`
- 大写宏：`OUT`
- 大写宏：`PARSE_HDR`
- 大写宏：`PARSE_HDR_SZ`
- 大写宏：`PARSE_TAIL`
- 大写宏：`PARSE_TAIL_SZ`
- 大写宏：`SQL`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_IOERR_NOMEM`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_SHARED_CACHE`
- 大写宏：`SQLITE_PREPARE_`
- 大写宏：`SQLITE_PREPARE_PERSISTENT`
- 大写宏：`SQLITE_SCHEMA`
- 大写宏：`SQLITE_TXN_NONE`
- 大写宏：`TESTONLY`
- 大写宏：`UTF`
- 外部类型：`Any`
- 外部类型：`Btree`
- 外部类型：`Check`
- 外部类型：`Close`
- 外部类型：`DB_SchemaLoaded`
- 外部类型：`Database`
- 外部类型：`DisableLookaside`
- 外部类型：`Do`
- 外部类型：`End`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`INITFLAG_AlterMask`
- 外部类型：`If`
- 外部类型：`InitData`
- 外部类型：`Initialization`
- 外部类型：`Length`
- 外部类型：`Loop`
- 外部类型：`Parse`
- 外部类型：`ParseCleanup`
- 外部类型：`Parsing`
- 外部类型：`Read`
- 外部类型：`Result`
- 外部类型：`Return`
- 外部类型：`SQLITE_WriteSchema`
- 外部类型：`The`
- 外部类型：`True`
- 外部类型：`Type`
- 外部类型：`Vdbe`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-aa6901fd10` → 本草稿移入 `cases/defect/auto-sqlite-aa6901fd10/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
