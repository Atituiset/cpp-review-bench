# auto-postgres-2882e0325a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#8e225d2ac57dd4e801e452c9398da4983c2d67b0](https://github.com/postgres/postgres/commit/8e225d2ac57dd4e801e452c9398da4983c2d67b0) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8e225d2ac57dd4e801e452c9398da4983c2d67b0 (https://github.com/postgres/postgres/commit/8e225d2ac57dd4e801e452c9398da4983c2d67b0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8e225d2ac57dd4e801e452c9398da4983c2d67b0 test_custom_stats: Add test for drop of variable-sized stats kind :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -655,6 +655,29 @@ test_custom_stats_var_drop(PG_FUNCTION_ARGS)
 	PG_RETURN_VOID();
 }
 
+/*
+ * Callback for pgstat_drop_matching_entries().
+ */
+static bool
+test_custom_stats_var_match_kind(PgStatShared_HashEntry *entry,
+								 Datum match_data)
+{
+	return entry->key.kind == PGSTAT_KIND_TEST_CUSTOM_VAR_STATS;
+}
+
+/*
+ * test_custom_stats_var_drop_all
+ *		Remove all custom statistic entries
+ */
+PG_FUNCTION_INFO_V1(test_custom_stats_var_drop_all);
+Datum
+test_custom_stats_var_drop_all(PG_FUNCTION_ARGS)
+{
+	pgstat_drop_matching_entries(test_custom_stats_var_match_kind, 0);
+
+	PG_RETURN_VOID();
+}
+
 /*
  * test_custom_stats_var_reset
  *		Reset all custom statistic entries
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`PG_RETURN_VOID`
- 外部类型：`Reset`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2882e0325a` → 本草稿移入 `cases/defect/auto-postgres-2882e0325a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
