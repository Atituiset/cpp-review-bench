# auto-postgres-261764eb26

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b6178c942ea7a85f72beb6a775d2793062a7ff8e](https://github.com/postgres/postgres/commit/b6178c942ea7a85f72beb6a775d2793062a7ff8e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b6178c942ea7a85f72beb6a775d2793062a7ff8e (https://github.com/postgres/postgres/commit/b6178c942ea7a85f72beb6a775d2793062a7ff8e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 8280；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b6178c942ea7a85f72beb6a775d2793062a7ff8e Allow LIMIT pushdown in postgres_fdw with empty ORDER BY pathkeys :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8277,11 +8277,17 @@ add_foreign_ordered_paths(PlannerInfo *root, RelOptInfo *input_rel,
 	if (input_rel->reloptkind == RELOPT_BASEREL ||
 		input_rel->reloptkind == RELOPT_JOINREL)
 	{
-		Assert(root->query_pathkeys == root->sort_pathkeys ||
-			   root->sort_pathkeys == NIL);
+		/* mark pushdown as safe when no sort is required for the ORDER BY */
+		if (root->sort_pathkeys == NIL)
+			fpinfo->pushdown_safe = true;
+		else
+		{
+			Assert(root->query_pathkeys == root->sort_pathkeys);
+
+			/* Safe to push down if the query_pathkeys are safe to push down */
+			fpinfo->pushdown_safe = ifpinfo->qp_is_pushdown_safe;
+		}
 
-		/* Safe to push down if the query_pathkeys are safe to push down */
-		fpinfo->pushdown_safe = ifpinfo->qp_is_pushdown_safe;
 		return;
 	}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`NIL`
- 大写宏：`RELOPT_BASEREL`
- 大写宏：`RELOPT_JOINREL`
- 外部类型：`Safe`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-261764eb26` → 本草稿移入 `cases/defect/auto-postgres-261764eb26/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
