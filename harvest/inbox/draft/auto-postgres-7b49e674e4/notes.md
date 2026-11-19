# auto-postgres-7b49e674e4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4280abf88409d09e5828c4642219df053220a650](https://github.com/postgres/postgres/commit/4280abf88409d09e5828c4642219df053220a650) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4280abf88409d09e5828c4642219df053220a650 (https://github.com/postgres/postgres/commit/4280abf88409d09e5828c4642219df053220a650)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 267；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4280abf88409d09e5828c4642219df053220a650 Fix pg_combinebackup handling of intermediate truncations :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -105,6 +105,7 @@ reconstruct_from_incremental_file(char *input_filename,
 	rfile	  **sourcemap;
 	off_t	   *offsetmap;
 	unsigned	block_length;
+	unsigned	truncation_block_length;
 	unsigned	sidx = n_prior_backups;
 	bool		full_copy_possible = true;
 	int			copy_source_index = -1;
@@ -128,6 +129,7 @@ reconstruct_from_incremental_file(char *input_filename,
 	latest_source = make_incremental_rfile(input_filename);
 	source[n_prior_backups] = latest_source;
 	block_length = find_reconstructed_block_length(latest_source);
+	truncation_block_length = latest_source->truncation_block_length;
 
 	/*
 	 * For each block in the output file, we need to know from which file we
@@ -224,7 +226,7 @@ reconstruct_from_incremental_file(char *input_filename,
 			 * later backup is one that we can't validate.
 			 */
 			blocklength = sb.st_size / BLCKSZ;
-			for (b = 0; b < latest_source->truncation_block_length; ++b)
+			for (b = 0; b < truncation_block_length; ++b)
 			{
 				if (sourcemap[b] == NULL && b < blocklength)
 				{
@@ -264,7 +266,7 @@ reconstruct_from_incremental_file(char *input_filename,
 		{
 			BlockNumber b = s->relative_block_numbers[i];
 
-			if (b < latest_source->truncation_block_length &&
+			if (b < truncation_block_length &&
 				sourcemap[b] == NULL)
 			{
 				sourcemap[b] = s;
@@ -278,6 +280,19 @@ reconstruct_from_incremental_file(char *input_filename,
 				full_copy_possible = false;
 			}
 		}
+
+		/*
+		 * Blocks at or beyond this incremental file's truncation_block_length
+		 * were either not present or truncated away before this backup, so
+		 * their contents in any earlier backup are stale and must not be
+		 * used. This also prevents copying an earlier full file whole even if
+		 * its size happens to match latest_source->truncation_block_length.
+		 */
+		if (s->truncation_block_length < truncation_block_length)
+		{
+			truncation_block_length = s->truncation_block_length;
+			full_copy_possible = false;
+		}
 	}
 
 	/*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`number`
- 外部函数：`open`
- 外部函数：`pfree`
- 外部函数：`pg_fatal`
- 外部函数：`pg_free`
- 外部函数：`pg_malloc0_array`
- 外部函数：`pg_malloc0_object`
- 外部函数：`pstrdup`
- 外部函数：`read`
- 大写宏：`BLCKSZ`
- 大写宏：`ENOENT`
- 大写宏：`INCREMENTAL_MAGIC`
- 大写宏：`NULL`
- 大写宏：`O_RDONLY`
- 大写宏：`PG_BINARY`
- 大写宏：`RELSEG_SIZE`
- 外部类型：`BlockNumber`
- 外部类型：`For`
- 外部类型：`Only`
- 外部类型：`Read`
- 外部类型：`Remember`
- 外部类型：`Round`
- 外部类型：`off_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-7b49e674e4` → 本草稿移入 `cases/defect/auto-postgres-7b49e674e4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
