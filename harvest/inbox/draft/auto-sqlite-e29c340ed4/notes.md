# auto-sqlite-e29c340ed4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d8371154c887fc190d0d11efc5ec221916fb8911](https://github.com/sqlite/sqlite/commit/d8371154c887fc190d0d11efc5ec221916fb8911) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d8371154c887fc190d0d11efc5ec221916fb8911 (https://github.com/sqlite/sqlite/commit/d8371154c887fc190d0d11efc5ec221916fb8911)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d8371154c887fc190d0d11efc5ec221916fb8911 Disallow the count-of-view optimization on compound queries if any arm of :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7496,6 +7496,7 @@ static void agginfoFree(sqlite3 *db, void *pArg){
 **   *  The outer query is a simple count(*) with no WHERE clause or other
 **      extraneous syntax.
 **   *  None of the subqueries are DISTINCT (forumpost/a860f5fb2e 2025-03-10)
+**   *  None of the subqueries use window functions
 **
 ** Return TRUE if the optimization is undertaken.
 */
@@ -7529,6 +7530,9 @@ static int countOfViewOptimization(Parse *pParse, Select *p){
     if( pSub->op!=TK_ALL && pSub->pPrior ) return 0;  /* Must be UNION ALL */
     if( pSub->pWhere ) return 0;                      /* No WHERE clause */
     if( pSub->pLimit ) return 0;                      /* No LIMIT clause */
+#ifndef SQLITE_OMIT_WINDOWFUNC
+    if( pSub->pWin ) return 0;                        /* No window functions */
+#endif
     if( pSub->selFlags & (SF_Aggregate|SF_Distinct) ){
        testcase( pSub->selFlags & SF_Aggregate );
        testcase( pSub->selFlags & SF_Distinct );
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`count`
- 外部函数：`testcase`
- 大写宏：`ALL`
- 大写宏：`DISTINCT`
- 大写宏：`LIMIT`
- 大写宏：`TK_ALL`
- 大写宏：`TRUE`
- 大写宏：`UNION`
- 大写宏：`WHERE`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`None`
- 外部类型：`Return`
- 外部类型：`SF_Aggregate`
- 外部类型：`SF_Distinct`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e29c340ed4` → 本草稿移入 `cases/defect/auto-sqlite-e29c340ed4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
