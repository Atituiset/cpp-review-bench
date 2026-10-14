# auto-sqlite-69e16d86f1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#9919c36b55bf5d37ad8eb67270c001b1d00db0e9](https://github.com/sqlite/sqlite/commit/9919c36b55bf5d37ad8eb67270c001b1d00db0e9) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9919c36b55bf5d37ad8eb67270c001b1d00db0e9 (https://github.com/sqlite/sqlite/commit/9919c36b55bf5d37ad8eb67270c001b1d00db0e9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 4214；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9919c36b55bf5d37ad8eb67270c001b1d00db0e9 Avoid incorrectly marking an index used by an OR-scan as a covering index leading to an "internal query planner error" e :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4211,7 +4211,9 @@ static int whereLoopAddBtree(
         pNew->wsFlags = WHERE_IDX_ONLY | WHERE_INDEXED;
       }else{
         m = pSrc->colUsed & pProbe->colNotIdxed;
-        if( pProbe->pPartIdxWhere ){
+        if( pProbe->pPartIdxWhere 
+         && (pWInfo->wctrlFlags & WHERE_OR_SUBCLAUSE)==0
+        ){
           wherePartIdxExpr(
               pWInfo->pParse, pProbe, pProbe->pPartIdxWhere, &m, 0, 0
           );
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3ColumnType`
- 外部函数：`sqlite3DbFreeNN`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3ExprAffinity`
- 外部函数：`sqlite3ExprAlloc`
- 外部函数：`sqlite3ExprAttachSubtrees`
- 外部函数：`sqlite3ExprCompareCollSeq`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ExprDup`
- 外部函数：`sqlite3ExprIsConstant`
- 外部函数：`sqlite3IsBinary`
- 外部函数：`sqlite3ParserAddCleanup`
- 外部函数：`strlen`
- 大写宏：`BMS`
- 大写宏：`FROM`
- 大写宏：`JT_LEFT`
- 大写宏：`JT_LTORJ`
- 大写宏：`JT_RIGHT`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`TK_AND`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_EQ`
- 大写宏：`TK_IS`
- 大写宏：`WHERE`
- 大写宏：`WHERE_IDX_ONLY`
- 大写宏：`WHERE_INDEXED`
- 外部类型：`Because`
- 外部类型：`Bitmask`
- 外部类型：`Column`
- 外部类型：`Cursor`
- 外部类型：`Expr`
- 外部类型：`Index`
- 外部类型：`IndexedExpr`
- 外部类型：`Mask`
- 外部类型：`Parse`
- 外部类型：`Partial`
- 外部类型：`SrcItem`
- 外部类型：`TF_Strict`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Token`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-69e16d86f1` → 本草稿移入 `cases/defect/auto-sqlite-69e16d86f1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
