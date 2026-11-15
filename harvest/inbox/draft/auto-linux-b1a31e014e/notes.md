# auto-linux-b1a31e014e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#23c62304dde29abe92013d85910d03a77dada35d](https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23c62304dde29abe92013d85910d03a77dada35d (https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1093；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23c62304dde29abe92013d85910d03a77dada35d Merge tag 'for-linus-7.4a-rc3-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/xen/tip :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -38,6 +38,7 @@
 #include <linux/sched.h>
 #include <linux/mm.h>
 #include <linux/slab.h>
+#include <linux/sched/task_stack.h>
 #include <linux/vmalloc.h>
 #include <linux/uaccess.h>
 #include <linux/io.h>
@@ -1063,6 +1064,9 @@ int gnttab_dma_alloc_pages(struct gnttab_dma_alloc_args *args)
 	size_t size;
 	int i, ret;
 
+	if (WARN_ON(xen_pv_domain()))
+		return -EINVAL;
+
 	if (args->nr_pages < 0 || args->nr_pages > (INT_MAX >> PAGE_SHIFT))
 		return -ENOMEM;
 
@@ -1090,11 +1094,27 @@ int gnttab_dma_alloc_pages(struct gnttab_dma_alloc_args *args)
 		xenmem_reservation_scrub_page(page);
 	}
 
-	xenmem_reservation_va_mapping_reset(args->nr_pages, args->pages);
-
 	ret = xenmem_reservation_decrease(args->nr_pages, args->frames);
 	if (ret != args->nr_pages) {
 		pr_debug("Failed to decrease reservation for DMA buffer\n");
+		if (ret >= 0) {
+			/* Free the part where decrease didn't work. */
+			size_t done = ret << PAGE_SHIFT;
+
+			if (args->coherent)
+				dma_free_coherent(args->dev, size - done,
+						  args->vaddr + done,
+						  args->dev_bus_addr + done);
+			else
+				dma_free_wc(args->dev, size - done,
+					    args->vaddr + done,
+					    args->dev_bus_addr + done);
+
+			if (!ret) /* Nothing else to do? */
+				return -EFAULT;
+
+			args->nr_pages = ret;
+		}
 		ret = -EFAULT;
 		goto fail;
 	}
@@ -1120,6 +1140,9 @@ int gnttab_dma_free_pages(struct gnttab_dma_alloc_args *args)
 	size_t size;
 	int i, ret;
 
+	if (WARN_ON(xen_pv_domain()))
+		return -EINVAL;
+
 	gnttab_pages_clear_private(args->nr_pages, args->pages);
 
 	for (i = 0; i < args->nr_pages; i++)
@@ -1128,14 +1151,16 @@ int gnttab_dma_free_pages(struct gnttab_dma_alloc_args *args)
 	ret = xenmem_reservation_increase(args->nr_pages, args->frames);
 	if (ret != args->nr_pages) {
 		pr_debug("Failed to increase reservation for DMA buffer\n");
-		ret = -EFAULT;
+		/* Can only free what has been re-filled. */
+		if (!ret)
+			return -ENOMEM;
+		if (ret > 0)
+			args->nr_pages = ret;
+		ret = -ENOMEM;
 	} else {
 		ret = 0;
 	}
 
-	xenmem_reservation_va_mapping_update(args->nr_pages, args->pages,
-					     args->frames);
-
 	size = args->nr_pages << PAGE_SHIFT;
 	if (args->coherent)
 		dma_free_coherent(args->dev, size,
@@ -1360,12 +1385,21 @@ static void unmap_refs_callback(int result,
 int gnttab_unmap_refs_sync(struct gntab_unmap_queue_data *item)
 {
 	struct unmap_refs_callback_data data;
+	bool on_stack = object_is_on_stack(item);
 
 	init_completion(&data.completion);
 	item->data = &data;
 	item->done = &unmap_refs_callback;
-	gnttab_unmap_refs_async(item);
+	/* blkback keeps @item, and so its retry work, on the stack. */
+	if (on_stack)
+		INIT_DELAYED_WORK_ONSTACK(&item->gnttab_work, gnttab_unmap_work);
+	else
+		INIT_DELAYED_WORK(&item->gnttab_work, gnttab_unmap_work);
+	item->age = 0;
+	__gnttab_unmap_refs_async(item);
 	wait_for_completion(&data.completion);
+	if (on_stack)
+		destroy_delayed_work_on_stack(&item->gnttab_work);
 
 	return data.result;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clear_foreign_p2m_mapping`
- 外部函数：`complete`
- 外部函数：`container_of`
- 外部函数：`dma_free_coherent`
- 外部函数：`done`
- 外部函数：`gnttab_unmap_refs`
- 外部函数：`init_completion`
- 外部函数：`kfree`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`page_count`
- 外部函数：`page_private`
- 外部函数：`pr_debug`
- 外部函数：`schedule_delayed_work`
- 外部函数：`set_page_private`
- 外部函数：`wait_for_completion`
- 外部函数：`xenmem_reservation_decrease`
- 外部函数：`xenmem_reservation_increase`
- 外部函数：`xenmem_reservation_scrub_page`
- 外部函数：`xenmem_reservation_va_mapping_reset`
- 外部函数：`xenmem_reservation_va_mapping_update`
- 大写宏：`BITS_PER_LONG`
- 大写宏：`DMA`
- 大写宏：`EFAULT`
- 大写宏：`ENOMEM`
- 大写宏：`INIT_DELAYED_WORK`
- 大写宏：`INT_MAX`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`UINT_MAX`
- 外部类型：`Failed`
- 外部类型：`GNTTABOP_unmap_grant_ref`
- 外部类型：`completion`
- 外部类型：`gntab_unmap_queue_data`
- 外部类型：`gnttab_unmap_grant_ref`
- 外部类型：`page`
- 外部类型：`size_t`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b1a31e014e` → 本草稿移入 `cases/defect/auto-linux-b1a31e014e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
