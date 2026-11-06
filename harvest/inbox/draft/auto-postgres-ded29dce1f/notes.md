# auto-postgres-ded29dce1f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f](https://github.com/postgres/postgres/commit/edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f (https://github.com/postgres/postgres/commit/edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 3868；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f Fix crash with single-input aggregates with multiple sort keys :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3865,11 +3865,11 @@ ExecBuildAggTrans(AggState *aggstate, AggStatePerPhase phase,
 			}
 			Assert(pertrans->numTransInputs == argno);
 		}
-		else if (pertrans->numInputs == 1)
+		else if (pertrans->numInputs == 1 && pertrans->numSortCols == 1)
 		{
 			/*
 			 * Non-presorted DISTINCT and/or ORDER BY case, with a single
-			 * column sorted on.
+			 * input column sorted on a single key.
 			 */
 			TargetEntry *source_tle =
 				(TargetEntry *) linitial(pertrans->aggref->args);
@@ -3888,7 +3888,7 @@ ExecBuildAggTrans(AggState *aggstate, AggStatePerPhase phase,
 		{
 			/*
 			 * Non-presorted DISTINCT and/or ORDER BY case, with multiple
-			 * columns sorted on.
+			 * input columns or sort keys.
 			 */
 			Datum	   *values = pertrans->sortslot->tts_values;
 			bool	   *nulls = pertrans->sortslot->tts_isnull;
@@ -4114,7 +4114,7 @@ ExecBuildAggTransCall(ExprState *state, AggState *aggstate,
 				scratch->opcode = EEOP_AGG_PLAIN_TRANS_BYREF;
 		}
 	}
-	else if (pertrans->numInputs == 1)
+	else if (pertrans->numInputs == 1 && pertrans->numSortCols == 1)
 		scratch->opcode = EEOP_AGG_ORDERED_TRANS_DATUM;
 	else
 		scratch->opcode = EEOP_AGG_ORDERED_TRANS_TUPLE;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`linitial`
- 大写宏：`DISTINCT`
- 大写宏：`EEOP_AGG_ORDERED_TRANS_DATUM`
- 大写宏：`EEOP_AGG_ORDERED_TRANS_TUPLE`
- 大写宏：`EEOP_AGG_PLAIN_TRANS_BYREF`
- 大写宏：`ORDER`
- 外部类型：`Datum`
- 外部类型：`Non`
- 外部类型：`TargetEntry`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ded29dce1f` → 本草稿移入 `cases/defect/auto-postgres-ded29dce1f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
