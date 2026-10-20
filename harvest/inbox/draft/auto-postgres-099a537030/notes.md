# auto-postgres-099a537030

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90](https://github.com/postgres/postgres/commit/7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90 (https://github.com/postgres/postgres/commit/7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 416；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90 Recompute allocChunkLimit when reusing an AllocSet context :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -321,6 +321,44 @@ AllocSetFreeIndex(Size size)
 	return idx;
 }
 
+/* ----------
+ * AllocSetFreeIndex -
+ *
+ *		Compute and return the multi-chunk block chunk size limit for the
+ *		given maximum block size.
+ * ----------
+ */
+static inline uint32
+AllocSetComputeChunkLimit(Size maxBlockSize)
+{
+	uint32		allocChunkLimit;
+
+	/*
+	 * Compute the allocation chunk size limit for this context.  It can't be
+	 * more than ALLOC_CHUNK_LIMIT because of the fixed number of freelists.
+	 * If maxBlockSize is small then requests exceeding the maxBlockSize, or
+	 * even a significant fraction of it, should be treated as large chunks
+	 * too.  For the typical case of maxBlockSize a power of 2, the chunk size
+	 * limit will be at most 1/8th maxBlockSize, so that given a stream of
+	 * requests that are all the maximum chunk size we will waste at most
+	 * 1/8th of the allocated space.
+	 *
+	 * Determine the maximum size that a chunk can be before we allocate an
+	 * entire AllocBlock dedicated for that chunk.  We set the absolute limit
+	 * of that size as ALLOC_CHUNK_LIMIT but we reduce it further so that we
+	 * can fit about ALLOC_CHUNK_FRACTION chunks this size on a maximally
+	 * sized block.  (We opt to keep allocChunkLimit a power-of-2 value
+	 * primarily for legacy reasons rather than calculating it so that exactly
+	 * ALLOC_CHUNK_FRACTION chunks fit on a maximally sized block.)
+	 */
+	allocChunkLimit = ALLOC_CHUNK_LIMIT;
+	while ((Size) (allocChunkLimit + ALLOC_CHUNKHDRSZ) >
+		   (Size) ((maxBlockSize - ALLOC_BLOCKHDRSZ) / ALLOC_CHUNK_FRACTION))
+		allocChunkLimit >>= 1;
+
+	return allocChunkLimit;
+}
+
 
 /*
  * Public routines
@@ -412,8 +450,18 @@ AllocSetContextCreateInternal(MemoryContext parent,
 			freelist->first_free = (AllocSet) set->header.nextchild;
 			freelist->num_free--;
 
-			/* Update its maxBlockSize; everything else should be OK */
-			set->maxBlockSize = maxBlockSize;
+			/* nextBlockSize should have been reset in AllocSetReset() */
+			Assert(set->nextBlockSize == set->initBlockSize);
+
+			if (set->maxBlockSize != maxBlockSize)
+			{
+				/*
+				 * Update its maxBlockSize and allocChunkLimit; everything
+				 * else should be OK.
+				 */
+				set->maxBlockSize = maxBlockSize;
+				set->allocChunkLimit = AllocSetComputeChunkLimit(maxBlockSize);
+			}
 
 			/* Reinitialize its header, installing correct name and parent */
 			MemoryContextCreate((MemoryContext) set,
@@ -495,28 +543,7 @@ AllocSetContextCreateInternal(MemoryContext parent,
 	set->nextBlockSize = (uint32) initBlockSize;
 	set->freeListIndex = freeListIndex;
 
-	/*
-	 * Compute the allocation chunk size limit for this context.  It can't be
-	 * more than ALLOC_CHUNK_LIMIT because of the fixed number of freelists.
-	 * If maxBlockSize is small then requests exceeding the maxBlockSize, or
-	 * even a significant fraction of it, should be treated as large chunks
-	 * too.  For the typical case of maxBlockSize a power of 2, the chunk size
-	 * limit will be at most 1/8th maxBlockSize, so that given a stream of
-	 * requests that are all the maximum chunk size we will waste at most
-	 * 1/8th of the allocated space.
-	 *
-	 * Determine the maximum size that a chunk can be before we allocate an
-	 * entire AllocBlock dedicated for that chunk.  We set the absolute limit
-	 * of that size as ALLOC_CHUNK_LIMIT but we reduce it further so that we
-	 * can fit about ALLOC_CHUNK_FRACTION chunks this size on a maximally
-	 * sized block.  (We opt to keep allocChunkLimit a power-of-2 value
-	 * primarily for legacy reasons rather than calculating it so that exactly
-	 * ALLOC_CHUNK_FRACTION chunks fit on a maximally sized block.)
-	 */
-	set->allocChunkLimit = ALLOC_CHUNK_LIMIT;
-	while ((Size) (set->allocChunkLimit + ALLOC_CHUNKHDRSZ) >
-		   (Size) ((maxBlockSize - ALLOC_BLOCKHDRSZ) / ALLOC_CHUNK_FRACTION))
-		set->allocChunkLimit >>= 1;
+	set->allocChunkLimit = AllocSetComputeChunkLimit(maxBlockSize);
 
 	/* Finally, do the type-independent 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`MAXALIGN`
- 外部类型：`AllocBlockData`
- 外部类型：`AllocSetContext`
- 外部类型：`Allocation`
- 外部类型：`Compute`
- 外部类型：`Determine`
- 外部类型：`Finally`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Info`
- 外部类型：`It`
- 外部类型：`MemoryChunk`
- 外部类型：`MemoryContext`
- 外部类型：`MemoryContextData`
- 外部类型：`Public`
- 外部类型：`Reinitialize`
- 外部类型：`Size`
- 外部类型：`Standard`
- 外部类型：`Update`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-099a537030` → 本草稿移入 `cases/defect/auto-postgres-099a537030/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
