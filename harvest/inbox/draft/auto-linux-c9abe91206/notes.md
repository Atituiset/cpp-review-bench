# auto-linux-c9abe91206

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c61e23ed14359e6289c08e55940dd3401fe27db0](https://github.com/torvalds/linux/commit/c61e23ed14359e6289c08e55940dd3401fe27db0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 43 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c61e23ed14359e6289c08e55940dd3401fe27db0 (https://github.com/torvalds/linux/commit/c61e23ed14359e6289c08e55940dd3401fe27db0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2353；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c61e23ed14359e6289c08e55940dd3401fe27db0 Merge tag 'drm-misc-fixes-2026-10-09' of https://gitlab.freedesktop.org/drm/misc/kernel into drm-fixes :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2337,7 +2337,7 @@ void pvr_mmu_op_context_destroy(struct pvr_mmu_op_context *op_ctx)
  * @ctx: MMU context associated with owning VM context.
  * @sgt: Scatter gather table containing pages pinned for use by this context.
  * @device_addr: Virtual device address at the start of the requested mapping.
- * @sgt_offset: Start offset of the requested device-virtual memory mapping.
+ * @sgt_offset: Offset into @sgt of the start of the requested mapping.
  * @size: Size in bytes of the requested device-virtual memory mapping. For an
  * unmapping, this should be zero so that no page tables are allocated.
  *
@@ -2350,7 +2350,6 @@ struct pvr_mmu_op_context *
 pvr_mmu_op_context_create(struct pvr_mmu_context *ctx, struct sg_table *sgt,
 			  u64 device_addr, u64 sgt_offset, u64 size)
 {
-	u64 start_addr = device_addr + sgt_offset;
 	int err;
 
 	struct pvr_mmu_op_context *op_ctx = kzalloc_obj(*op_ctx);
@@ -2365,18 +2364,18 @@ pvr_mmu_op_context_create(struct pvr_mmu_context *ctx, struct sg_table *sgt,
 
 	if (size) {
 		/*
-		 * The number of page table objects we need to prealloc is
-		 * indicated by the mapping size, start address and the sizes
-		 * of the areas mapped per PT or PD. The range calculation is
-		 * identical to that for the index into a table for a device
-		 * address, so we reuse those functions here.
+		 * The page tables needed are set by the device-virtual range
+		 * being mapped: one level 1 table per 1GiB region and one
+		 * level 0 table per 2MiB region the range touches. Tables that
+		 * already exist are not consumed, so this is an upper bound.
 		 */
-		const u32 l1_start_idx = pvr_page_table_l2_idx(start_addr);
-		const u32 l1_end_idx = pvr_page_table_l2_idx(start_addr + size);
-		const u32 l1_count = l1_end_idx - l1_start_idx + 1;
-		const u32 l0_start_idx = pvr_page_table_l1_idx(start_addr);
-		const u32 l0_end_idx = pvr_page_table_l1_idx(start_addr + size);
-		const u32 l0_count = l0_end_idx - l0_start_idx + 1;
+		const u64 last_addr = device_addr + size - 1;
+		const u64 l1_count =
+			(last_addr >> ROGUE_MMUCTRL_VADDR_PC_INDEX_SHIFT) -
+			(device_addr >> ROGUE_MMUCTRL_VADDR_PC_INDEX_SHIFT) + 1;
+		const u64 l0_count =
+			(last_addr >> ROGUE_MMUCTRL_VADDR_PD_INDEX_SHIFT) -
+			(device_addr >> ROGUE_MMUCTRL_VADDR_PD_INDEX_SHIFT) + 1;
 
 		/*
 		 * Alloc and push page table entries until we have enough of
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`entries`
- 外部函数：`has`
- 外部函数：`kzalloc_obj`
- 外部函数：`pvr_mmu_op_context_require_sync`
- 外部函数：`pvr_mmu_op_context_sync_partial`
- 外部函数：`pvr_page_table_l0_remove`
- 外部函数：`pvr_page_table_l1_remove`
- 外部函数：`pvr_page_table_l2_remove`
- 外部函数：`table`
- 大写宏：`MMU`
- 大写宏：`PVR`
- 大写宏：`PVR_MMU_SYNC_LEVEL_0`
- 大写宏：`PVR_MMU_SYNC_LEVEL_1`
- 大写宏：`PVR_MMU_SYNC_LEVEL_2`
- 大写宏：`PVR_MMU_SYNC_LEVEL_NONE`
- 大写宏：`ROGUE_MMUCTRL_ENTRIES_PC_VALUE`
- 大写宏：`ROGUE_MMUCTRL_ENTRIES_PD_VALUE`
- 大写宏：`ROGUE_MMUCTRL_VADDR_PC_INDEX_CLRMSK`
- 大写宏：`ROGUE_MMUCTRL_VADDR_PC_INDEX_SHIFT`
- 大写宏：`ROGUE_MMUCTRL_VADDR_PD_INDEX_CLRMSK`
- 大写宏：`ROGUE_MMUCTRL_VADDR_PD_INDEX_SHIFT`
- 外部类型：`Alloc`
- 外部类型：`Collects`
- 外部类型：`Data`
- 外部类型：`For`
- 外部类型：`Index`
- 外部类型：`Linked`
- 外部类型：`Only`
- 外部类型：`Pointer`
- 外部类型：`Preallocated`
- 外部类型：`Scatter`
- 外部类型：`Size`
- 外部类型：`Start`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Used`
- 外部类型：`Virtual`
- 外部类型：`When`
- 外部类型：`dma_addr_t`
- 外部类型：`page`
- 外部类型：`pvr_device`
- 外部类型：`sg_table`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c9abe91206` → 本草稿移入 `cases/defect/auto-linux-c9abe91206/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
