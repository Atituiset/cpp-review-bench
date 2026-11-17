# auto-postgres-2191003922

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#60cdae088b6d86e38059c32a0a1fd66e46de2389](https://github.com/postgres/postgres/commit/60cdae088b6d86e38059c32a0a1fd66e46de2389) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 200 |
| 编译错误数（gcc syntax-only） | 61（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #60cdae088b6d86e38059c32a0a1fd66e46de2389 (https://github.com/postgres/postgres/commit/60cdae088b6d86e38059c32a0a1fd66e46de2389)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4541；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 60cdae088b6d86e38059c32a0a1fd66e46de2389 plpgsql: Clarify the LIMIT 1 example in the too-many-rows hint :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4538,7 +4538,7 @@ exec_stmt_execsql(PLpgSQL_execstate *estate,
 						(errcode(ERRCODE_TOO_MANY_ROWS),
 						 errmsg("query returned more than one row"),
 						 errdetail ? errdetail_internal("parameters: %s", errdetail) : 0,
-						 errhint("Make sure the query returns a single row, or use LIMIT 1.")));
+						 errhint("Make sure the query returns a single row, for example by using LIMIT 1 with SELECT.")));
 			}
 			/* Put the first result row into the target */
 			exec_move_row(estate, target, tuptab->vals[0], tuptab->tupdesc);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assign_simple_var`
- 外部函数：`datumTransfer`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail_internal`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`eval_mcontext_alloc`
- 外部函数：`exec_cast_value`
- 外部函数：`exec_move_row`
- 外部函数：`exec_move_row_from_datum`
- 外部函数：`expand_array`
- 外部函数：`get_eval_mcontext`
- 外部函数：`type_is_rowtype`
- 外部函数：`value`
- 大写宏：`ERRCODE_DATATYPE_MISMATCH`
- 大写宏：`ERRCODE_NULL_VALUE_NOT_ALLOWED`
- 大写宏：`ERRCODE_TOO_MANY_ROWS`
- 大写宏：`ERROR`
- 大写宏：`FIRST`
- 大写宏：`LIMIT`
- 大写宏：`LXID`
- 大写宏：`MUST`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PLPGSQL_DTYPE_PROMISE`
- 大写宏：`PLPGSQL_DTYPE_REC`
- 大写宏：`PLPGSQL_DTYPE_RECFIELD`
- 大写宏：`PLPGSQL_DTYPE_ROW`
- 大写宏：`PLPGSQL_DTYPE_VAR`
- 大写宏：`PLPGSQL_PROMISE_NONE`
- 大写宏：`RECORD`
- 大写宏：`VARATT_IS_EXTERNAL_EXPANDED_RW`
- 外部类型：`Also`
- 外部类型：`But`
- 外部类型：`Datum`
- 外部类型：`ExpandedRecordHeader`
- 外部类型：`ExprState`
- 外部类型：`If`
- 外部类型：`LocalTransactionId`
- 外部类型：`Make`
- 外部类型：`NULLs`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Oid`
- 外部类型：`PLpgSQL_datum`
- 外部类型：`PLpgSQL_execstate`
- 外部类型：`PLpgSQL_rec`
- 外部类型：`PLpgSQL_recfield`
- 外部类型：`PLpgSQL_row`
- 外部类型：`PLpgSQL_var`
- 外部类型：`PLpgSQL_variable`
- 外部类型：`Put`
- 外部类型：`Set`
- 外部类型：`Source`
- 外部类型：`Target`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2191003922` → 本草稿移入 `cases/defect/auto-postgres-2191003922/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
