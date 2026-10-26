# auto-linux-62b7938016

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#126ad898a6e97fcb4c3fbe6d979829f72b8c348c](https://github.com/torvalds/linux/commit/126ad898a6e97fcb4c3fbe6d979829f72b8c348c) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 167 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #126ad898a6e97fcb4c3fbe6d979829f72b8c348c (https://github.com/torvalds/linux/commit/126ad898a6e97fcb4c3fbe6d979829f72b8c348c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 845；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 126ad898a6e97fcb4c3fbe6d979829f72b8c348c Merge tag 'liveupdate-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/liveupdate/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -12,6 +12,7 @@
 
 #include <linux/cleanup.h>
 #include <linux/cma.h>
+#include <linux/crash_dump.h>
 #include <linux/kmemleak.h>
 #include <linux/count_zeros.h>
 #include <linux/kasan.h>
@@ -42,12 +43,12 @@
 /*
  * This is the minimal alignment required by deferred struct page init.
  * deferred_init_memmap_chunk frees memory to the buddy allocator, which looks
- * at the neighboring pages (up to MAX_PAGE_ORDER) to merge them.
- * If KHO scratch is not aligned to that value, buddy can access uninitialized
- * struct pages, which can cause a crash.
+ * at the neighboring pages (up to MAX_PAGE_ORDER) to merge them. If KHO bootmem
+ * is not aligned to that value, buddy can access uninitialized struct pages,
+ * which can cause a crash.
  */
-#define SCRATCH_ALIGNMENT_BYTES (PAGE_SIZE * MAX_ORDER_NR_PAGES)
-static_assert(SCRATCH_ALIGNMENT_BYTES >= CMA_MIN_ALIGNMENT_BYTES);
+#define BOOTMEM_ALIGNMENT_BYTES (PAGE_SIZE * MAX_ORDER_NR_PAGES)
+static_assert(BOOTMEM_ALIGNMENT_BYTES >= CMA_MIN_ALIGNMENT_BYTES);
 
 /* The magic token for preserved pages */
 #define KHO_PAGE_MAGIC 0x4b484f50U /* ASCII for 'KHOP' */
@@ -97,7 +98,7 @@ static struct kho_out kho_out = {
 
 struct kho_in {
 	phys_addr_t fdt_phys;
-	phys_addr_t scratch_phys;
+	phys_addr_t bootmem_phys;
 	char previous_release[__NEW_UTS_LEN + 1];
 	u32 kexec_count;
 	struct kho_debugfs dbg;
@@ -657,34 +658,34 @@ static void __init *kho_get_mem_map(const void *fdt)
 
 /*
  * With KHO enabled, memory can become fragmented because KHO regions may
- * be anywhere in physical address space. The scratch regions give us a
+ * be anywhere in physical address space. The bootmem regions give us a
  * safe zones that we will never see KHO allocations from. This is where we
- * can later safely load our new kexec images into and then use the scratch
+ * can later safely load our new kexec images into and then use the bootmem
  * area for early allocations that happen before page allocator is
  * initialized.
  */
-struct kho_scratch *kho_scratch;
-unsigned int kho_scratch_cnt;
+struct kho_bootmem *kho_bootmem;
+unsigned int kho_bootmem_cnt;
 
 /*
- * The scratch areas are scaled by default as percent of memory allocated from
+ * The bootmem areas are scaled by default as percent of memory allocated from
  * memblock. A user can override the scale with command line parameter:
  *
- * kho_scratch=N%
+ * kho_bootmem=N%
  *
  * It is also possible to explicitly define size for a lowmem, a global and
- * per-node scratch areas:
+ * per-node bootmem areas:
  *
- * kho_scratch=l[KMG],n[KMG],m[KMG]
+ * kho_bootmem=l[KMG],n[KMG],m[KMG]
  *
  * The explicit size definition takes precedence over scale definition.
  */
-static unsigned int scratch_scale __initdata = 200;
-static phys_addr_t scratch_size_global __initdata;
-static phys_addr_t scratch_size_pernode __initdata;
-static phys_addr_t scratch_size_lowmem __initdata;
+static unsigned int bootmem_scale __initdata = 200;
+static phys_addr_t bootmem_size_global __initdata;
+static phys_addr_t bootmem_size_pernode __initdata;
+static phys_addr_t bootmem_size_lowmem __initdata;
 
-static int __init kho_parse_scratch_size(char *p)
+static int __init kho_parse_bootmem_size(char *p)
 {
 	size_t len;
 	unsigned long sizes[3];
@@ -708,9 +709,9 @@ static int __init kho_parse_scratch_size(char *p)
 			return -EINVAL;
 
 		memcpy(s_scale, p, len - 1);
-		ret = kstrtouint(s_scale, 10, &scratch_scale);
+		ret = kstrtouint(s_scale, 10, &bootmem_scale);
 		if (!ret)
-			pr_notice("scratch scale is %d%%\n", scratch_scale);
+			pr_notice("bootmem scale is %d%%\n", bootmem_scale);
 		return ret;
 	}
 
@@ -738,171 +739,180 @@ static int __init kho_parse_scratch_size(char *p)
 	if (*p)
 		return -EINVAL;
 
-	scratch_size_lowmem = sizes[0];
-	scratch_size_global = sizes[1];
-	scratch_size_pernode = sizes[2];
-	scratch_scale = 0;
+	bootmem_size_lowmem = sizes[0];
+	bootmem_size_global = sizes[1];
+	bootmem_size_pernode = sizes[2];
+	bootmem_scale = 0;
 
-	pr_notice
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__clear_bit`
- 外部函数：`__free_pages`
- 外部函数：`__set_bit`
- 外部函数：`__va`
- 外部函数：`bitmap`
- 外部函数：`early_memremap`
- 外部函数：`early_memunmap`
- 外部函数：`early_param`
- 外部函数：`early_pfn_to_nid`
- 外部函数：`fdt_getprop`
- 外部函数：`fls64`
- 外部函数：`folio_address`
- 外部函数：`folio_alloc`
- 外部函数：`folio_order`
- 外部函数：`folio_pfn`
- 外部函数：`folio_put`
- 外部函数：`free_area_init`
- 外部函数：`free_page`
- 外部函数：`free_reserved_area`
- 外部函数：`func`
- 外部函数：`get_order`
- 外部函数：`get_unaligned`
- 外部函数：`get_zeroed_page`
- 外部函数：`guard`
- 外部函数：`init_cma_reserved_pageblock`
- 外部函数：`init_deferred_page`
- 外部函数：`int`
- 外部函数：`kexec_add_buffer`
- 外部函数：`kho_ext_walk_leaf`
- 外部函数：`kho_extend_scratch`
- 外部函数：`kmemleak_ignore_phys`
- 外部函数：`kstrtouint`
- 外部函数：`leaf`
- 外部函数：`memblock_add`
- 外部函数：`memblock_alloc`
- 外部函数：`memblock_alloc_range_nid`
- 外部函数：`memblock_end_of_DRAM`
- 外部函数：`memblock_free`
- 外部函数：`memblock_mark_kho_scratch`
- 外部函数：`memblock_phys_alloc`
- 外部函数：`memblock_phys_alloc_range`
- 外部函数：`memblock_phys_free`
- 外部函数：`memblock_reserve`
- 外部函数：`memblock_reserved_hugetlb_size`
- 外部函数：`memblock_reserved_kern_size`
- 外部函数：`memblock_reserved_mark_noinit`
- 外部函数：`memblock_set_kho_scratch_only`
- 外部函数：`memcpy`
- 外部函数：`might_sleep`
- 外部函数：`mutex_init`
- 外部函数：`node`
- 外部函数：`nodes_weight`
- 外部函数：`pages`
- 外部函数：`pfn_to_page`
- 外部函数：`phys_to_virt`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`pr_notice`
- 外部函数：`pr_warn`
- 外部函数：`reserve_regions`
- 外部函数：`round_up`
- 外部函数：`scratch`
- 外部函数：`slab_is_available`
- 外部函数：`unlikely`
- 外部函数：`virt_to_folio`
- 外部函数：`virt_to_phys`
- 大写宏：`ARCH_LOW_ADDRESS_LIMIT`
- 大写宏：`ASCII`
- 大写宏：`BITS_PER_BYTE`
- 大写宏：`CMA`
- 大写宏：`CMA_MIN_ALIGNMENT_BYTES`
- 大写宏：`CONFIG_DEFERRED_STRUCT_PAGE_INIT`
- 大写宏：`CONFIG_KEXEC_HANDOVER_DEBUG`
- 大写宏：`DRAM`
- 大写宏：`E2BIG`
- 大写宏：`EADDRNOTAVAIL`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`ERANGE`
- 大写宏：`ERR_PTR`
- 大写宏：`FDT`
- 大写宏：`FIXME`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`KEXEC_BUF_MEM_UNKNOWN`
- 大写宏：`KEXEC_TYPE_CRASH`
- 大写宏：`KHO`
- 大写宏：`KHOP`
- 大写宏：`KHO_BITMAP_SIZE_LOG2`
- 大写宏：`KHO_FDT_MEMORY_MAP_PROP_NAME`
- 大写宏：`KHO_RADIX_KEY_WIDTH`
- 大写宏：`KHO_TABLE_SIZE_LOG2`
- 大写宏：`KHO_TREE_MAX_DEPTH`
- 大写宏：`KMG`
- 大写宏：`MAX_ORDER_NR_PAGES`
- 大写宏：`MAX_PAGE_ORDER`
- 大写宏：`MEMBLOCK_ALLOC_ACCESSIBLE`
- 大写宏：`MEMBLOCK_ALLOC_ANYWHERE`
- 大写宏：`MEMBLOCK_KHO_SCRATCH`
- 大写宏：`MIGRATE_CMA`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`NUMA_NO_NODE`
- 大写宏：`N_MEMORY`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PFN`
- 大写宏：`PFN_PHYS`
- 大写宏：`PHYS_PFN`
- 大写宏：`PRESENT`
- 大写宏：`SZ_1G`
- 大写宏：`SZ_64K`
- 大写宏：`ULONG_MAX`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Attempting`
- 外部类型：`Called`
- 外部类型：`Capture`
- 外部类型：`Clear`
- 外部类型：`Do`
- 外部类型：`Extend`
- 外部类型：`Failed`
- 外部类型：`Go`
- 外部类型：`Handle`
- 外部类型：`HugeTLB`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Keys`
- 外部类型：`Loop`
- 外部类型：`Makes`
- 外部类型：`Mark`
- 外部类型：`Marked`
- 外部类型：`Newly`
- 外部类型：`Next`
- 外部类型：`Now`
- 外部类型：`Order`
- 外部类型：`Remove`
- 外部类型：`Reserve`
- 外部类型：`Round`
- 外部类型：`Scratch`
- 外部类型：`Shifted`
- 外部类型：`Skip`
- 外部类型：`Subsequent`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Try`
- 外部类型：`Walk`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`With`
- 外部类型：`folio`
- 外部类型：`kexec_buf`
- 外部类型：`kho_debugfs`
- 外部类型：`kho_radix_leaf`
- 外部类型：`kho_radix_node`
- 外部类型：`kho_radix_tree`
- 外部类型：`kho_radix_walk_cb`
- 外部类型：`kho_scratch`
- 外部类型：`kimage`
- 外部类型：`lock_class_key`
- 外部类型：`mutex`
- 外部类型：`page`
- 外部类型：`pages`
- 外部类型：`phys_addr_t`
- 外部类型：`resource`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-62b7938016` → 本草稿移入 `cases/defect/auto-linux-62b7938016/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
