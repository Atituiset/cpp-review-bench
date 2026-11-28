# auto-sqlite-032a986f1c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#54df57f9a328a78b3e61ab44dec6d73fdc5a1819](https://github.com/sqlite/sqlite/commit/54df57f9a328a78b3e61ab44dec6d73fdc5a1819) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #54df57f9a328a78b3e61ab44dec6d73fdc5a1819 (https://github.com/sqlite/sqlite/commit/54df57f9a328a78b3e61ab44dec6d73fdc5a1819)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 987；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 54df57f9a328a78b3e61ab44dec6d73fdc5a1819 Correctly identify correlated subqueries that are deeply nexted in :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -978,13 +978,17 @@ static int sqlite3ReturningSubqueryVarSelect(Walker *NotUsed, Expr *pExpr){
 
 
 /*
-** If the SELECT references the table pWalker->u.pTab, then do two things:
+** If the SELECT directly references the table pWalker->u.pTab, then do
+** two things:
 **
 **    (1) Mark the SELECT as as SF_Correlated.
 **    (2) Set pWalker->eCode to non-zero so that the caller will know
 **        that (1) has happened.
 */
-static int sqlite3ReturningSubqueryCorrelated(Walker *pWalker, Select *pSelect){
+static int sqlite3ReturningSubqueryCorrelated1(
+  Walker *pWalker,
+  Select *pSelect
+){
   int i;
   SrcList *pSrc;
   assert( pSelect!=0 );
@@ -1001,6 +1005,27 @@ static int sqlite3ReturningSubqueryCorrelated(Walker *pWalker, Select *pSelect){
   return WRC_Continue;
 }
 
+/*
+** Mark the SELECT as SF_Correlated if it, or any subquery nested inside
+** it, references the table pWalker->u.pTab.  This way, an outer subquery
+** that merely contains a correlated subquery is not treated as constant.
+*/
+static int sqlite3ReturningSubqueryCorrelated(
+  Walker *pWalker,
+  Select *pSelect
+){
+  Walker w2;
+  w2 = *pWalker;
+  w2.eCode = 0;
+  w2.xSelectCallback = sqlite3ReturningSubqueryCorrelated1;
+  sqlite3WalkSelect(&w2, pSelect);
+  if( w2.eCode ){
+    pSelect->selFlags |= SF_Correlated;
+    pWalker->eCode = 1;
+  }
+  return WRC_Continue;
+}
+
 /*
 ** Scan the expression list that is the argument to RETURNING looking
 ** for subqueries that depend on the table which is being modified in the
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3ReturningSubqueryCorrelated`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 外部类型：`If`
- 外部类型：`Mark`
- 外部类型：`SF_Correlated`
- 外部类型：`Scan`
- 外部类型：`Select`
- 外部类型：`Set`
- 外部类型：`SrcList`
- 外部类型：`WRC_Continue`
- 外部类型：`Walker`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-032a986f1c` → 本草稿移入 `cases/defect/auto-sqlite-032a986f1c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
