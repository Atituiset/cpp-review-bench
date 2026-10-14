# auto-postgres-760761d0af

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0d6103e537e1299046e5906149f9a7d2108be2cf](https://github.com/postgres/postgres/commit/0d6103e537e1299046e5906149f9a7d2108be2cf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0d6103e537e1299046e5906149f9a7d2108be2cf (https://github.com/postgres/postgres/commit/0d6103e537e1299046e5906149f9a7d2108be2cf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1639；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0d6103e537e1299046e5906149f9a7d2108be2cf Give antijoins made from sublinks a relid :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1635,19 +1635,16 @@ deconstruct_recurse(PlannerInfo *root, Node *jtnode,
 									child_domain->jd_relids);
 				jtitem->qualscope = bms_union(left_item->qualscope,
 											  right_item->qualscope);
-				/* caution: ANTI join derived from SEMI will lack rtindex */
-				if (j->rtindex != 0)
-				{
-					parent_domain->jd_relids =
-						bms_add_member(parent_domain->jd_relids,
-									   j->rtindex);
-					jtitem->qualscope = bms_add_member(jtitem->qualscope,
+				Assert(j->rtindex != 0);
+				parent_domain->jd_relids =
+					bms_add_member(parent_domain->jd_relids,
+								   j->rtindex);
+				jtitem->qualscope = bms_add_member(jtitem->qualscope,
+												   j->rtindex);
+				root->outer_join_rels = bms_add_member(root->outer_join_rels,
 													   j->rtindex);
-					root->outer_join_rels = bms_add_member(root->outer_join_rels,
-														   j->rtindex);
-					mark_rels_nulled_by_join(root, j->rtindex,
-											 right_item->qualscope);
-				}
+				mark_rels_nulled_by_join(root, j->rtindex,
+										 right_item->qualscope);
 				jtitem->inner_join_rels = bms_union(left_item->inner_join_rels,
 													right_item->inner_join_rels);
 				jtitem->left_rels = left_item->qualscope;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_is_member`
- 外部函数：`bms_next_member`
- 外部函数：`bms_union`
- 大写宏：`ANTI`
- 大写宏：`NULL`
- 大写宏：`RTE`
- 大写宏：`RTE_GROUP`
- 大写宏：`SEMI`
- 外部类型：`Index`
- 外部类型：`PlannerInfo`
- 外部类型：`RelOptInfo`
- 外部类型：`Relids`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-760761d0af` → 本草稿移入 `cases/defect/auto-postgres-760761d0af/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
