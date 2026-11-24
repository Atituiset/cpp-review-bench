# auto-sqlite-30f09065c2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#95635a25d5a058dac504649debd970ba2f465801](https://github.com/sqlite/sqlite/commit/95635a25d5a058dac504649debd970ba2f465801) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #95635a25d5a058dac504649debd970ba2f465801 (https://github.com/sqlite/sqlite/commit/95635a25d5a058dac504649debd970ba2f465801)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 95635a25d5a058dac504649debd970ba2f465801 New OP_NoWriter opcode blocks some DDL statements if there are concurrent :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4049,6 +4049,20 @@ int sqlite3HasExplicitNulls(Parse *pParse, ExprList *pList){
   return 0;
 }
 
+/*
+** Use the OP_NoWriter opcode to cause the current program to abort
+** if there are concurrent writers on the table iRoot of schema iDb.
+**
+** Calls to this routine must be followed by VdbeCoverage(v) in order to
+** measure coverage of the inserted OP_NoWriter opcode.
+*/
+void sqlite3FailIfConcurrentWriter(Parse *pParse, Table *pTab){
+  Vdbe *v = sqlite3GetVdbe(pParse);
+  int iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
+  assert( v!=0 || pParse->nErr>0 );
+  if( v ) sqlite3VdbeAddOp2(v, OP_NoWriter, (int)pTab->tnum, iDb);
+}
+
 /*
 ** Create a new index for an SQL table.  pName1.pName2 is the name of the index
 ** and pTblList is the name of the table that is to be indexed.  Both will
@@ -4559,6 +4573,7 @@ void sqlite3CreateIndex(
       if( v==0 ) goto exit_create_index;
 
       sqlite3BeginWriteOperation(pParse, 1, iDb);
+      sqlite3FailIfConcurrentWriter(pParse, pTab); VdbeCoverage(v);
 
       /* Create the rootpage for the index using CreateIndex. But before
       ** doing so, code a Noop instruction and store its address in
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3BtreeOpen`
- 外部函数：`sqlite3BtreeSetPageSize`
- 外部函数：`sqlite3OomFault`
- 外部函数：`sqlite3ParseToplevel`
- 外部函数：`sqlite3SchemaMutexHeld`
- 大写宏：`OMIT_TEMPDB`
- 大写宏：`SQL`
- 大写宏：`SQLITE_MAX_DB`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OPEN_CREATE`
- 大写宏：`SQLITE_OPEN_DELETEONCLOSE`
- 大写宏：`SQLITE_OPEN_EXCLUSIVE`
- 大写宏：`SQLITE_OPEN_READWRITE`
- 大写宏：`SQLITE_OPEN_TEMP_DB`
- 外部类型：`Both`
- 外部类型：`Btree`
- 外部类型：`But`
- 外部类型：`Create`
- 外部类型：`CreateIndex`
- 外部类型：`Noop`
- 外部类型：`Parse`
- 外部类型：`See`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-30f09065c2` → 本草稿移入 `cases/defect/auto-sqlite-30f09065c2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
