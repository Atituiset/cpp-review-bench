# auto-linux-88f0014e23

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9c7e3d59107e2daf204bf6bf87b6c448ca89defe](https://github.com/torvalds/linux/commit/9c7e3d59107e2daf204bf6bf87b6c448ca89defe) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 48 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9c7e3d59107e2daf204bf6bf87b6c448ca89defe (https://github.com/torvalds/linux/commit/9c7e3d59107e2daf204bf6bf87b6c448ca89defe)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 34；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9c7e3d59107e2daf204bf6bf87b6c448ca89defe Merge tag 'hyperv-next-signed-20261028' of git://git.kernel.org/pub/scm/linux/kernel/git/hyperv/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -9,135 +9,177 @@
 #include <linux/export.h>
 #include <asm/mshyperv.h>
 
+#define HV_DEPOSIT_MAX 512
+#define HV_DEPOSIT_INP_MAX ((HV_HYP_PAGE_SIZE -  \
+	offsetof(struct hv_deposit_memory, gpa_page_list)) / sizeof(u64))
+
 /*
- * See struct hv_deposit_memory. The first u64 is partition ID, the rest
- * are GPAs.
+ * Allocate free pages for deposit to hypervisor. pfna[] must be large enough
+ * to hold HV_DEPOSIT_INP_MAX (511) pages. If num_pages is 512, return last
+ * pfn in lastpfn. If @single, then it must be a single allocation (not split
+ * over multiple contiguous ranges).
+ *
+ * Returns: number of pages allocated or -ENOMEM
  */
-#define HV_DEPOSIT_MAX (HV_HYP_PAGE_SIZE / sizeof(u64) - 1)
-
-/* Deposits exact number of pages. Must be called with interrupts enabled.  */
-int hv_call_deposit_pages(int node, u64 partition_id, u32 num_pages)
+static int hv_alloc_dep_pages(int node, u64 *pfna, u64 *lastpfnp, int num_pages,
+			      bool single)
 {
-	struct page **pages, *page;
-	int *counts;
-	int num_allocations;
-	int i, j, page_count;
-	int order;
-	u64 status;
-	int ret;
-	u64 base_pfn;
-	struct hv_deposit_memory *input_page;
-	unsigned long flags;
-
-	if (num_pages > HV_DEPOSIT_MAX)
-		return -E2BIG;
-	if (!num_pages)
-		return 0;
-
-	/* One buffer for page pointers and counts */
-	page = alloc_page(GFP_KERNEL);
-	if (!page)
-		return -ENOMEM;
-	pages = page_address(page);
+	struct page *page;
+	int num_allocd, count = 0;
 
-	counts = kzalloc_objs(int, HV_DEPOSIT_MAX);
-	if (!counts) {
-		free_page((unsigned long)pages);
-		return -ENOMEM;
-	}
+	/* Published ABI, enforce its immutability. */
+	BUILD_BUG_ON(HV_DEPOSIT_INP_MAX != 511);
 
-	/* Allocate all the pages before disabling interrupts */
-	i = 0;
+	if (num_pages > HV_DEPOSIT_MAX ||
+	    (num_pages == HV_DEPOSIT_MAX && lastpfnp == NULL))
+		return -EINVAL;
 
 	while (num_pages) {
 		/* Find highest order we can actually allocate */
-		order = 31 - __builtin_clz(num_pages);
+		int order = 31 - __builtin_clz(num_pages);
+		gfp_t gfp_flags = GFP_KERNEL;
+
+		if (!single)
+			gfp_flags |= __GFP_NOWARN;
 
 		while (1) {
-			pages[i] = alloc_pages_node(node, GFP_KERNEL, order);
-			if (pages[i])
+			page = alloc_pages_node(node, gfp_flags, order);
+			if (page || order == 0 || single)
 				break;
-			if (!order) {
-				ret = -ENOMEM;
-				num_allocations = i;
-				goto err_free_allocations;
-			}
-			--order;
+
+			order--;
 		}
 
-		split_page(pages[i], order);
-		counts[i] = 1 << order;
-		num_pages -= counts[i];
-		i++;
-	}
-	num_allocations = i;
+		if (page == NULL)
+			break;
 
-	local_irq_save(flags);
+		split_page(page, order);
+		num_allocd = 1 << order;
+		num_pages -= num_allocd;
 
-	input_page = *this_cpu_ptr(hyperv_pcpu_input_arg);
+		while (num_allocd && count < HV_DEPOSIT_INP_MAX) {
+			pfna[count++] = page_to_pfn(page++);
+			num_allocd--;
+		}
 
-	input_page->partition_id = partition_id;
+		if (num_allocd-- && count == HV_DEPOSIT_INP_MAX) {
+			*lastpfnp = page_to_pfn(page);
+			count++;
+			break;
+		}
+	}
+
+	return count ? count : -ENOMEM;
+}
 
-	/* Populate gpa_page_list - these will fit on the input page */
-	for (i = 0, page_count = 0; i < num_allocations; ++i) {
-		base_pfn = page_to_pfn(pages[i]);
-		for (j = 0; j < counts[i]; ++j, ++page_count)
-			input_page->gpa_page_list[page_count] = base_pfn + j;
+/*
+ * Deposit memory in the hypervisor. Even if @contiguous is false, a contiguous
+ * 2M worth of pfns is utmost desired for performance reasons. But short of
+ * that, we deposit whatever contiguous chunks we can get. If @contiguous is
+ * true, then the entire range has to be physically contiguous. Note, in that
+ * case, upon withdrawl, hypervisor could return any page in between the range,
+ * so we must split that also. Lastly, HV_MAX_CONTIGUOUS_ALLOCATION_PAGES is
+ * not guaranteed to always be power of 2.
+ */
+static int hv_call_deposit_pages(int node, u64 partition_id, bool contiguous)
+{
+	struct hv_deposit_memory *hc_input;
+	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__builtin_clz`
- 外部函数：`__free_page`
- 外部函数：`alloc_page`
- 外部函数：`alloc_pages_node`
- 外部函数：`free_page`
- 外部函数：`hv_call_deposit_pages`
- 外部函数：`hv_do_rep_hypercall`
- 外部函数：`hv_result`
- 外部函数：`hv_result_success`
- 外部函数：`hv_result_to_errno`
- 外部函数：`hv_root_partition`
- 外部函数：`hv_status_err`
- 外部函数：`kfree`
- 外部函数：`kzalloc_objs`
- 外部函数：`local_irq_restore`
- 外部函数：`local_irq_save`
- 外部函数：`page_address`
- 外部函数：`page_to_pfn`
- 外部函数：`pfn_to_page`
- 外部函数：`split_page`
- 外部函数：`this_cpu_ptr`
- 大写宏：`E2BIG`
- 大写宏：`ENOMEM`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HVCALL_DEPOSIT_MEMORY`
- 大写宏：`HV_HYP_PAGE_SIZE`
- 大写宏：`HV_MAX_CONTIGUOUS_ALLOCATION_PAGES`
- 大写宏：`HV_PARTITION_ID_SELF`
- 大写宏：`HV_STATUS_INSUFFICIENT_CONTIGUOUS_MEMORY`
- 大写宏：`HV_STATUS_INSUFFICIENT_CONTIGUOUS_ROOT_MEMORY`
- 大写宏：`HV_STATUS_INSUFFICIENT_MEMORY`
- 大写宏：`HV_STATUS_INSUFFICIENT_ROOT_MEMORY`
- 大写宏：`NULL`
- 外部类型：`Allocate`
- 外部类型：`Deposits`
- 外部类型：`Find`
- 外部类型：`GPAs`
- 外部类型：`It`
- 外部类型：`Must`
- 外部类型：`One`
- 外部类型：`Populate`
- 外部类型：`Root`
- 外部类型：`See`
- 外部类型：`The`
- 外部类型：`Unexpected`
- 外部类型：`VPs`
- 外部类型：`hv_deposit_memory`
- 外部类型：`page`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-88f0014e23` → 本草稿移入 `cases/defect/auto-linux-88f0014e23/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
