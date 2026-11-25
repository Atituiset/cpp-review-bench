# auto-postgres-cda44250e7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#470d9948965835a8c3bfb3ac102384bcc84a604c](https://github.com/postgres/postgres/commit/470d9948965835a8c3bfb3ac102384bcc84a604c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 72（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #470d9948965835a8c3bfb3ac102384bcc84a604c (https://github.com/postgres/postgres/commit/470d9948965835a8c3bfb3ac102384bcc84a604c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 918；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 470d9948965835a8c3bfb3ac102384bcc84a604c Fix -Wcast-qual warning in guc_funcs.c :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -915,7 +915,7 @@ show_all_settings(PG_FUNCTION_ARGS)
 	for (int i = 0; i < num_vars; i++)
 	{
 		struct config_generic *conf = guc_vars[i];
-		char	   *values[NUM_PG_SETTINGS_ATTS];
+		const char *values[NUM_PG_SETTINGS_ATTS];
 		HeapTuple	tuple;
 
 		/* skip if marked NO_SHOW_ALL or if not visible to current user */
@@ -924,10 +924,17 @@ show_all_settings(PG_FUNCTION_ARGS)
 			continue;
 
 		/* extract values for the current variable */
-		GetConfigOptionValues(conf, (const char **) values);
+		GetConfigOptionValues(conf, values);
 
-		/* build a tuple and store it */
-		tuple = BuildTupleFromCStrings(attinmeta, values);
+		/*
+		 * build a tuple
+		 *
+		 * (BuildTupleFromCStrings() takes char ** but does not modify the
+		 * strings, so it's safe to cast away const here.)
+		 */
+		tuple = BuildTupleFromCStrings(attinmeta, unconstify(char **, values));
+
+		/* store it */
 		tuplestore_puttuple(rsinfo->setResult, tuple);
 	}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`any`
- 外部函数：`config_enum_get_options`
- 外部函数：`config_enum_lookup_by_value`
- 外部函数：`get_config_unit_name`
- 外部函数：`has_privs_of_role`
- 外部函数：`pstrdup`
- 外部函数：`snprintf`
- 外部函数：`tuplestore_puttuple`
- 大写宏：`GUC_PENDING_RESTART`
- 大写宏：`GUC_SHOW_IN_OCTAL`
- 大写宏：`NOTE`
- 大写宏：`NO_SHOW_ALL`
- 大写宏：`NULL`
- 大写宏：`PGC_BOOL`
- 大写宏：`PGC_ENUM`
- 大写宏：`PGC_INT`
- 大写宏：`PGC_REAL`
- 大写宏：`PGC_STRING`
- 大写宏：`PGC_S_FILE`
- 大写宏：`ROLE_PG_READ_ALL_SETTINGS`
- 外部类型：`For`
- 外部类型：`GucContext_Names`
- 外部类型：`GucSource_Names`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`ShowGUCOption`
- 外部类型：`config_bool`
- 外部类型：`config_enum`
- 外部类型：`config_generic`
- 外部类型：`config_int`
- 外部类型：`config_real`
- 外部类型：`config_string`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cda44250e7` → 本草稿移入 `cases/defect/auto-postgres-cda44250e7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
