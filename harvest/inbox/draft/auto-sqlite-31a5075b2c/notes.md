# auto-sqlite-31a5075b2c 🟢 零依赖候选

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#fc96a3c4530bbc66274133d3727a556821dddce6](https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 0（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fc96a3c4530bbc66274133d3727a556821dddce6 (https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fc96a3c4530bbc66274133d3727a556821dddce6 Add the tree-tracker optimization. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1936,7 +1936,7 @@ struct sqlite3 {
 #define SQLITE_QueryFlattener 0x00000001 /* Query flattening */
 #define SQLITE_WindowFunc     0x00000002 /* Use xInverse for window functions */
 #define SQLITE_GroupByOrder   0x00000004 /* GROUPBY cover of ORDERBY */
-                           /* 0x00000008 -- Available for reuse */
+#define SQLITE_JoinMiss       0x00000008 /* Unwind join loops on a lookup miss*/
 #define SQLITE_DistinctOpt    0x00000010 /* DISTINCT using indexes */
 #define SQLITE_CoverIdxScan   0x00000020 /* Covering index scans */
 #define SQLITE_OrderByIdxJoin 0x00000040 /* ORDER BY of joins via index */
@@ -5287,9 +5287,7 @@ int sqlite3ExprIsConstantOrFunction(Expr*, u8);
 int sqlite3ExprIsConstantOrGroupBy(Parse*, Expr*, ExprList*);
 int sqlite3ExprIsSingleTableConstraint(Expr*,const SrcList*,int,int);
 int sqlite3ExprListIsConstant(Parse *pParse, ExprList *pList, int bNoIs);
-#ifdef SQLITE_ENABLE_CURSOR_HINTS
 int sqlite3ExprContainsSubquery(Expr*);
-#endif
 int sqlite3ExprIsInteger(const Expr*, int*, Parse*, int);
 int sqlite3ExprCanBeNull(const Expr*);
 int sqlite3ExprNeedsNoAffinityChange(const Expr*, char);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3ExprCanBeNull`
- 外部函数：`sqlite3ExprContainsSubquery`
- 外部函数：`sqlite3ExprIsConstantOrGroupBy`
- 外部函数：`sqlite3ExprIsInteger`
- 外部函数：`sqlite3ExprIsSingleTableConstraint`
- 外部函数：`sqlite3ExprListIsConstant`
- 外部函数：`sqlite3ExprNeedsNoAffinityChange`
- 大写宏：`DISTINCT`
- 大写宏：`GROUPBY`
- 大写宏：`ORDER`
- 大写宏：`ORDERBY`
- 大写宏：`SQLITE_ENABLE_CURSOR_HINTS`
- 外部类型：`Available`
- 外部类型：`Covering`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`Parse`
- 外部类型：`Query`
- 外部类型：`SQLITE_CoverIdxScan`
- 外部类型：`SQLITE_DistinctOpt`
- 外部类型：`SQLITE_GroupByOrder`
- 外部类型：`SQLITE_OrderByIdxJoin`
- 外部类型：`SQLITE_QueryFlattener`
- 外部类型：`SQLITE_WindowFunc`
- 外部类型：`SrcList`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-31a5075b2c` → 本草稿移入 `cases/defect/auto-sqlite-31a5075b2c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
