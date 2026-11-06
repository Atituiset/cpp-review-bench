# auto-postgres-463f3cce18

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#69b20fdd20de68b0f0eaa855669a6657c65c8533](https://github.com/postgres/postgres/commit/69b20fdd20de68b0f0eaa855669a6657c65c8533) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 77 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #69b20fdd20de68b0f0eaa855669a6657c65c8533 (https://github.com/postgres/postgres/commit/69b20fdd20de68b0f0eaa855669a6657c65c8533)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 69b20fdd20de68b0f0eaa855669a6657c65c8533 Fix infinite loop in first_value()/nth_value() with IGNORE NULLS. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3466,6 +3466,9 @@ ignorenulls_getfuncarginframe(WindowObject winobj, int argno,
 		int			inframe;
 		int			v;
 
+		/* we may skip many rows without fetching any of them */
+		CHECK_FOR_INTERRUPTS();
+
 		/*
 		 * Check apparent out of frame case.  We need to do this because we
 		 * may not call window_gettupleslot before row_is_in_frame, which
@@ -3474,6 +3477,18 @@ ignorenulls_getfuncarginframe(WindowObject winobj, int argno,
 		if (abs_pos < 0)
 			goto out_of_frame;
 
+		/*
+		 * Likewise, stop at the end of the partition, which row_is_in_frame
+		 * expects its caller to have checked.  It cannot always detect that
+		 * itself: for a frame ending at UNBOUNDED FOLLOWING with EXCLUDE
+		 * GROUP or EXCLUDE TIES and no ORDER BY, it reports every row other
+		 * than the current one as excluded, wherever it is, so we would
+		 * never reach the end of the loop.
+		 */
+		spool_tuples(winstate, abs_pos);
+		if (abs_pos >= winstate->spooled_rows)
+			goto out_of_frame;
+
 		/* check whether row is in frame */
 		inframe = row_is_in_frame(winobj, abs_pos, slot, true);
 		if (inframe == -1)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`are_peers`
- 外部函数：`outerPlanState`
- 外部函数：`pg_add_s64_overflow`
- 外部函数：`tuplestore_in_memory`
- 外部函数：`tuplestore_puttupleslot`
- 外部函数：`update_frametailpos`
- 外部函数：`update_grouptailpos`
- 外部函数：`window_gettupleslot`
- 大写宏：`FIXME`
- 大写宏：`FRAMEOPTION_END_CURRENT_ROW`
- 大写宏：`FRAMEOPTION_END_OFFSET`
- 大写宏：`FRAMEOPTION_END_OFFSET_PRECEDING`
- 大写宏：`FRAMEOPTION_EXCLUDE_CURRENT_ROW`
- 大写宏：`FRAMEOPTION_EXCLUDE_GROUP`
- 大写宏：`FRAMEOPTION_EXCLUDE_TIES`
- 大写宏：`FRAMEOPTION_GROUPS`
- 大写宏：`FRAMEOPTION_RANGE`
- 大写宏：`FRAMEOPTION_ROWS`
- 大写宏：`FRAMEOPTION_START_UNBOUND`
- 大写宏：`ORDER`
- 大写宏：`WINDOWAGG_PASSTHROUGH`
- 大写宏：`WINDOWAGG_PASSTHROUGH_STRICT`
- 大写宏：`WINDOWAGG_RUN`
- 大写宏：`XXX`
- 外部类型：`Check`
- 外部类型：`ExprContext`
- 外部类型：`First`
- 外部类型：`Here`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`MemoryContext`
- 外部类型：`Must`
- 外部类型：`Okay`
- 外部类型：`Otherwise`
- 外部类型：`PlanState`
- 外部类型：`Remember`
- 外部类型：`Since`
- 外部类型：`Still`
- 外部类型：`TupleTableSlot`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`WindowAgg`
- 外部类型：`WindowAggState`
- 外部类型：`WindowObject`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-463f3cce18` → 本草稿移入 `cases/defect/auto-postgres-463f3cce18/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
