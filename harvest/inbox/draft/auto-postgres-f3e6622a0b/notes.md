# auto-postgres-f3e6622a0b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c712b0d49b971fe7cf521c057bc7adb63b17760c](https://github.com/postgres/postgres/commit/c712b0d49b971fe7cf521c057bc7adb63b17760c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c712b0d49b971fe7cf521c057bc7adb63b17760c (https://github.com/postgres/postgres/commit/c712b0d49b971fe7cf521c057bc7adb63b17760c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 357；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c712b0d49b971fe7cf521c057bc7adb63b17760c Allow unknown-size shmem attachments in single-user mode :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -348,33 +348,27 @@ ShmemRequestInternal(ShmemStructOpts *options, ShmemRequestKind kind)
 	MemoryContext oldcontext;
 	ShmemRequest *request;
 
+	/* Check that we're in the right state */
+	if (shmem_request_state != SRS_REQUESTING)
+		elog(ERROR, "ShmemRequestStruct can only be called from a shmem_request callback");
+
 	/* Check the options */
 	if (options->name == NULL)
 		elog(ERROR, "shared memory request is missing 'name' option");
 
-	if (IsUnderPostmaster)
-	{
-		if (options->size <= 0 && options->size != SHMEM_ATTACH_UNKNOWN_SIZE)
-			elog(ERROR, "invalid size %zd for shared memory request for \"%s\"",
-				 options->size, options->name);
-	}
-	else
+	if (options->size == SHMEM_ATTACH_UNKNOWN_SIZE)
 	{
-		if (options->size == SHMEM_ATTACH_UNKNOWN_SIZE)
+		if (ShmemIndex == NULL)
 			elog(ERROR, "SHMEM_ATTACH_UNKNOWN_SIZE cannot be used during startup");
-		if (options->size <= 0)
-			elog(ERROR, "invalid size %zd for shared memory request for \"%s\"",
-				 options->size, options->name);
 	}
+	else if (options->size <= 0)
+		elog(ERROR, "invalid size %zd for shared memory request for \"%s\"",
+			 options->size, options->name);
 
 	if (options->alignment != 0 && pg_nextpower2_size_t(options->alignment) != options->alignment)
 		elog(ERROR, "invalid alignment %zu for shared memory request for \"%s\"",
 			 options->alignment, options->name);
 
-	/* Check that we're in the right state */
-	if (shmem_request_state != SRS_REQUESTING)
-		elog(ERROR, "ShmemRequestStruct can only be called from a shmem_request callback");
-
 	/* Check that it's not already registered in this process */
 	foreach_ptr(ShmemRequest, existing, pending_shmem_requests)
 	{
@@ -790,6 +784,8 @@ void
 ResetShmemAllocator(void)
 {
 	Assert(!IsUnderPostmaster);
+	ShmemAllocator = NULL;
+	ShmemIndex = NULL;
 	shmem_request_state = SRS_INITIAL;
 
 	pending_shmem_requests = NIL;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`pg_nextpower2_size_t`
- 大写宏：`ERROR`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`SHMEM_ATTACH_UNKNOWN_SIZE`
- 大写宏：`SRS_INITIAL`
- 大写宏：`SRS_REQUESTING`
- 外部类型：`Check`
- 外部类型：`IsUnderPostmaster`
- 外部类型：`MemoryContext`
- 外部类型：`ShmemIndexEnt`
- 外部类型：`ShmemRequest`
- 外部类型：`ShmemRequestKind`
- 外部类型：`ShmemRequestStruct`
- 外部类型：`ShmemStructOpts`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f3e6622a0b` → 本草稿移入 `cases/defect/auto-postgres-f3e6622a0b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
