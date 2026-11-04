# auto-postgres-b36612fcf8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#52b41da0be54f5df6708f95dc8bda73b8ae7c563](https://github.com/postgres/postgres/commit/52b41da0be54f5df6708f95dc8bda73b8ae7c563) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #52b41da0be54f5df6708f95dc8bda73b8ae7c563 (https://github.com/postgres/postgres/commit/52b41da0be54f5df6708f95dc8bda73b8ae7c563)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 8280；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 52b41da0be54f5df6708f95dc8bda73b8ae7c563 Fix incorrect Assert in add_foreign_ordered_paths :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8268,20 +8268,20 @@ add_foreign_ordered_paths(PlannerInfo *root, RelOptInfo *input_rel,
 	merge_fdw_options(fpinfo, ifpinfo, NULL);
 
 	/*
-	 * If the input_rel is a base or join relation, we would already have
-	 * considered pushing down the final sort to the remote server when
-	 * creating pre-sorted foreign paths for that relation, because the
-	 * query_pathkeys is set to the root->sort_pathkeys in that case (see
-	 * standard_qp_callback()).
+	 * If the input_rel is a base or join relation,
+	 * add_paths_with_pathkeys_for_rel() already considered the creation of
+	 * paths to suit query_pathkeys.  sort_pathkeys can be NIL here despite
+	 * the query having an ORDER BY clause, and query_pathkeys can be set to
+	 * setop_pathkeys (see standard_qp_callback()).
 	 */
 	if (input_rel->reloptkind == RELOPT_BASEREL ||
 		input_rel->reloptkind == RELOPT_JOINREL)
 	{
-		Assert(root->query_pathkeys == root->sort_pathkeys);
+		Assert(root->query_pathkeys == root->sort_pathkeys ||
+			   root->sort_pathkeys == NIL);
 
-		/* Safe to push down if the query_pathkeys is safe to push down */
+		/* Safe to push down if the query_pathkeys are safe to push down */
 		fpinfo->pushdown_safe = ifpinfo->qp_is_pushdown_safe;
-
 		return;
 	}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`standard_qp_callback`
- 大写宏：`EXPLAIN`
- 大写宏：`FDW`
- 大写宏：`NULL`
- 大写宏：`RELOPT_BASEREL`
- 大写宏：`RELOPT_JOINREL`
- 外部类型：`Copy`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Merge`
- 外部类型：`PgFdwRelationInfo`
- 外部类型：`Safe`
- 外部类型：`Set`
- 外部类型：`This`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b36612fcf8` → 本草稿移入 `cases/defect/auto-postgres-b36612fcf8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
