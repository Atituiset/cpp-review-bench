# auto-sqlite-612872aab3

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
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #95635a25d5a058dac504649debd970ba2f465801 (https://github.com/sqlite/sqlite/commit/95635a25d5a058dac504649debd970ba2f465801)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 95635a25d5a058dac504649debd970ba2f465801 New OP_NoWriter opcode blocks some DDL statements if there are concurrent :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -495,6 +495,9 @@ void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef){
     int isStrict = pTab->tabFlags & TF_Strict;
     int isGen = (pCol->colFlags & COLFLAG_GENERATED)!=0;
 
+    /* No concurrent writers of the table being changed */
+    sqlite3FailIfConcurrentWriter(pParse, pTab); VdbeCoverage(v);
+
     /* Make sure the schema version is at least 3.  But do not upgrade
     ** from less than 3 to 4, as that will corrupt any preexisting DESC
     ** index.
@@ -3045,6 +3048,9 @@ void sqlite3AlterSetNotNull(
   pCons = pFirst->z;
   nCons = alterRtrimFragment(pParse->db, pCons, pParse->sLastToken.z - pCons);
 
+  /* No concurrent writers of table being changed */
+  sqlite3FailIfConcurrentWriter(pParse, pTab); VdbeCoverage(pParse->pVdbe);
+
   /* Search for a constraint violation. Throw an exception if one is found. */
   sqlite3NestedParse(pParse,
       "SELECT sqlite_fail('constraint failed', %d) "
@@ -3166,6 +3172,9 @@ void sqlite3AlterAddConstraint(
     sqlite3DbFree(pParse->db, zName);
   }
 
+  /* No concurrent writers of the table being changed */
+  sqlite3FailIfConcurrentWriter(pParse, pTab); VdbeCoverage(pParse->pVdbe);
+
   /* Search for a constraint violation. Throw an exception if one is found. */
   sqlite3NestedParse(pParse,
       "SELECT sqlite_fail('constraint failed', %d) "
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3GetToken`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3NestedParse`
- 外部函数：`sqlite_fail`
- 大写宏：`COLFLAG_GENERATED`
- 大写宏：`DESC`
- 大写宏：`OOM`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`TK_COMMENT`
- 大写宏：`TK_ILLEGAL`
- 大写宏：`TK_SEMI`
- 大写宏：`TK_SPACE`
- 外部类型：`Buffer`
- 外部类型：`But`
- 外部类型：`Ignore`
- 外部类型：`Make`
- 外部类型：`Search`
- 外部类型：`Size`
- 外部类型：`TF_Strict`
- 外部类型：`Throw`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-612872aab3` → 本草稿移入 `cases/defect/auto-sqlite-612872aab3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
