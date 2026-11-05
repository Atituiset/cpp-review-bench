# auto-postgres-a9834be82b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b](https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b (https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 676；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b Add stats kind filter to pgstat_drop_matching_entries() :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -635,6 +635,24 @@ test_custom_stats_var_update(PG_FUNCTION_ARGS)
 	PG_RETURN_VOID();
 }
 
+/*
+ * test_custom_stats_var_match_entry
+ *		Match custom variable stats entries by objid
+ *
+ * Callback for pgstat_drop_matching_entries().  Verifies that it filters
+ * entries by kind before invoking the callback.
+ */
+static bool
+test_custom_stats_var_match_entry(PgStatShared_HashEntry *p, Datum match_data)
+{
+	if (p->key.kind != PGSTAT_KIND_TEST_CUSTOM_VAR_STATS)
+		elog(ERROR,
+			 "unexpected stats kind for custom stats match callback: %u",
+			 p->key.kind);
+
+	return p->key.objid == DatumGetUInt64(match_data);
+}
+
 /*
  * test_custom_stats_var_drop
  *		Remove custom statistic entry
@@ -656,6 +674,29 @@ test_custom_stats_var_drop(PG_FUNCTION_ARGS)
 }
 
 /*
+ * test_custom_stats_var_drop_matching
+ *		Remove custom statistic entry by matching objid
+ *
+ * Drops the named statistic using pgstat_drop_matching_entries().
+ */
+PG_FUNCTION_INFO_V1(test_custom_stats_var_drop_matching);
+Datum
+test_custom_stats_var_drop_matching(PG_FUNCTION_ARGS)
+{
+	char	   *stat_name = text_to_cstring(PG_GETARG_TEXT_PP(0));
+	uint64		stat_idx = PGSTAT_CUSTOM_VAR_STATS_IDX(stat_name);
+
+	pgstat_drop_matching_entries(PGSTAT_KIND_TEST_CUSTOM_VAR_STATS,
+								 test_custom_stats_var_match_entry,
+								 UInt64GetDatum(stat_idx));
+
+	PG_RETURN_VOID();
+}
+
+/*
+ * test_custom_stats_var_match_kind
+ *		Match custom variable stats entries by kind
+ *
  * Callback for pgstat_drop_matching_entries().
  */
 static bool
@@ -673,7 +714,9 @@ PG_FUNCTION_INFO_V1(test_custom_stats_var_drop_all);
 Datum
 test_custom_stats_var_drop_all(PG_FUNCTION_ARGS)
 {
-	pgstat_drop_matching_entries(test_custom_stats_var_match_kind, 0);
+	pgstat_drop_matching_entries(PGSTAT_KIND_INVALID,
+								 test_custom_stats_var_match_kind,
+								 0);
 
 	PG_RETURN_VOID();
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`test_custom_stats_var_match_kind`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_RETURN_VOID`
- 外部类型：`Callback`
- 外部类型：`Datum`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`Remove`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-a9834be82b` → 本草稿移入 `cases/defect/auto-postgres-a9834be82b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
