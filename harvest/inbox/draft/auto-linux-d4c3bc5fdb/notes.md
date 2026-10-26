# auto-linux-d4c3bc5fdb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#394023fbe34fccec5d4db2576169d21e14c8d3d4](https://github.com/torvalds/linux/commit/394023fbe34fccec5d4db2576169d21e14c8d3d4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 138 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #394023fbe34fccec5d4db2576169d21e14c8d3d4 (https://github.com/torvalds/linux/commit/394023fbe34fccec5d4db2576169d21e14c8d3d4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2075；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 394023fbe34fccec5d4db2576169d21e14c8d3d4 Merge tag 'memblock-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/mm/memblock :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -155,11 +155,6 @@ struct memblock_type physmem = {
  */
 static __refdata struct memblock_type *memblock_memory = &memblock.memory;
 
-#define for_each_memblock_type(i, memblock_type, rgn)			\
-	for (i = 0, rgn = &memblock_type->regions[0];			\
-	     i < memblock_type->cnt;					\
-	     i++, rgn = &memblock_type->regions[i])
-
 #define memblock_dbg(fmt, ...)						\
 	do {								\
 		if (memblock_debug)					\
@@ -591,6 +586,29 @@ static void __init_memblock memblock_insert_region(struct memblock_type *type,
 	type->total_size += size;
 }
 
+/**
+ * __memblock_search - Find the first region index where rend > base
+ * @type: memblock type to search
+ * @base: base physical address of the candidate range
+ *
+ * Return: the first region index that could potentially overlap @base.
+ */
+static int __init_memblock __memblock_search(struct memblock_type *type,
+						  phys_addr_t base)
+{
+	int mid, low = 0;
+	int high = type->cnt;
+
+	while (low < high) {
+		mid = (low + high) / 2;
+		if (type->regions[mid].base + type->regions[mid].size <= base)
+			low = mid + 1;
+		else
+			high = mid;
+	}
+	return low;
+}
+
 /**
  * memblock_add_range - add new memblock region
  * @type: memblock type to add new region into
@@ -614,8 +632,7 @@ static int __init_memblock memblock_add_range(struct memblock_type *type,
 	bool insert = false;
 	phys_addr_t obase = base;
 	phys_addr_t end = base + memblock_cap_size(base, &size);
-	int idx, nr_new, start_rgn = -1, end_rgn;
-	struct memblock_region *rgn;
+	int idx, nr_new, start_rgn = -1, end_rgn = 0;
 
 	if (!size)
 		return 0;
@@ -651,7 +668,8 @@ static int __init_memblock memblock_add_range(struct memblock_type *type,
 	base = obase;
 	nr_new = 0;
 
-	for_each_memblock_type(idx, type, rgn) {
+	for (idx = __memblock_search(type, base); idx < type->cnt; idx++) {
+		struct memblock_region *rgn = &type->regions[idx];
 		phys_addr_t rbase = rgn->base;
 		phys_addr_t rend = rbase + rgn->size;
 
@@ -814,8 +832,6 @@ static int __init_memblock memblock_isolate_range(struct memblock_type *type,
 					int *start_rgn, int *end_rgn)
 {
 	phys_addr_t end = base + memblock_cap_size(base, &size);
-	int idx;
-	struct memblock_region *rgn;
 
 	*start_rgn = *end_rgn = 0;
 
@@ -827,7 +843,8 @@ static int __init_memblock memblock_isolate_range(struct memblock_type *type,
 		if (memblock_double_array(type, base, size) < 0)
 			return -ENOMEM;
 
-	for_each_memblock_type(idx, type, rgn) {
+	for (int idx = __memblock_search(type, base); idx < type->cnt; idx++) {
+		struct memblock_region *rgn = &type->regions[idx];
 		phys_addr_t rbase = rgn->base;
 		phys_addr_t rend = rbase + rgn->size;
 
@@ -1659,7 +1676,6 @@ phys_addr_t __init memblock_phys_alloc_try_nid(phys_addr_t size, phys_addr_t ali
  * @min_addr: the lower bound of the memory region to allocate (phys address)
  * @max_addr: the upper bound of the memory region to allocate (phys address)
  * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
- * @exact_nid: control the allocation fall back to other nodes
  *
  * Allocates memory block using memblock_alloc_range_nid() and
  * converts the returned physical address to virtual.
@@ -1675,7 +1691,7 @@ phys_addr_t __init memblock_phys_alloc_try_nid(phys_addr_t size, phys_addr_t ali
 static void * __init memblock_alloc_internal(
 				phys_addr_t size, phys_addr_t align,
 				phys_addr_t min_addr, phys_addr_t max_addr,
-				int nid, bool exact_nid)
+				int nid)
 {
 	phys_addr_t alloc;
 
@@ -1684,50 +1700,19 @@ static void * __init memblock_alloc_internal(
 		max_addr = memblock.current_limit;
 
 	alloc = memblock_alloc_range_nid(size, align, min_addr, max_addr, nid,
-					exact_nid);
+					false);
 
 	/* retry allocation without lower limit */
 	if (!alloc && min_addr)
 		alloc = memblock_alloc_range_nid(size, align, 0, max_addr, nid,
-						exact_nid);
+						false);
 
 	if (!alloc)
 		return NULL;
 
 	return phys_to_virt(alloc);
 }
 
-/**
- * memblock_alloc_exact_nid_raw - allocate boot memory block o
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__pa`
- 外部函数：`__va`
- 外部函数：`accept_memory`
- 外部函数：`alloc_page`
- 外部函数：`atomic_long_set`
- 外部函数：`choose_memblock_flags`
- 外部函数：`clamp`
- 外部函数：`deferred_pages_enabled`
- 外部函数：`dump_stack`
- 外部函数：`early_pgtable_alloc`
- 外部函数：`fdt_begin_node`
- 外部函数：`fdt_create`
- 外部函数：`fdt_end_node`
- 外部函数：`fdt_finish`
- 外部函数：`fdt_finish_reservemap`
- 外部函数：`fdt_property`
- 外部函数：`fdt_property_string`
- 外部函数：`fdt_totalsize`
- 外部函数：`for_each_online_pgdat`
- 外部函数：`free_reserved_page`
- 外部函数：`information`
- 外部函数：`kasan_init`
- 外部函数：`kasan_reset_tag`
- 外部函数：`kfree`
- 外部函数：`kho_add_subtree`
- 外部函数：`kho_is_enabled`
- 外部函数：`kho_preserve_pages`
- 外部函数：`kho_remove_subtree`
- 外部函数：`kho_unpreserve_pages`
- 外部函数：`kmalloc`
- 外部函数：`kmemleak_alloc_phys`
- 外部函数：`kmemleak_free_part_phys`
- 外部函数：`kzalloc_node`
- 外部函数：`late_initcall`
- 外部函数：`max`
- 外部函数：`max_t`
- 外部函数：`memblock_alloc_exact_nid_raw`
- 外部函数：`memblock_alloc_internal`
- 外部函数：`memblock_bottom_up`
- 外部函数：`memblock_dbg`
- 外部函数：`memblock_get_region_node`
- 外部函数：`memblock_phys_alloc`
- 外部函数：`memblock_reserve_kern`
- 外部函数：`memblock_set_region_node`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`numa_valid_node`
- 外部函数：`page_address`
- 外部函数：`page_to_virt`
- 外部函数：`panic`
- 外部函数：`pfn_to_page`
- 外部函数：`phys_to_page`
- 外部函数：`phys_to_virt`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`pr_warn_ratelimited`
- 外部函数：`put_page`
- 外部函数：`round_down`
- 外部函数：`round_up`
- 外部函数：`slab_is_available`
- 外部函数：`strlen`
- 外部函数：`virt_to_phys`
- 大写宏：`AMD`
- 大写宏：`BUG_ON`
- 大写宏：`CONFIG_ARCH_KEEP_MEMBLOCK`
- 大写宏：`CONFIG_NUMA`
- 大写宏：`ENOMEM`
- 大写宏：`FDT`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GFP_NOWAIT`
- 大写宏：`IS_ENABLED`
- 大写宏：`KHO`
- 大写宏：`MAX_NUMNODES`
- 大写宏：`MEMBLOCK_ALLOC_ACCESSIBLE`
- 大写宏：`MEMBLOCK_ALLOC_NOLEAKTRACE`
- 大写宏：`MEMBLOCK_KHO_FDT`
- 大写宏：`MEMBLOCK_KHO_NODE_COMPATIBLE`
- 大写宏：`MEMBLOCK_KHO_SCRATCH`
- 大写宏：`MEMBLOCK_MIRROR`
- 大写宏：`MEMBLOCK_NONE`
- 大写宏：`MEMBLOCK_RSRV_KERN`
- 大写宏：`NULL`
- 大写宏：`NUMA_NO_NODE`
- 大写宏：`PAGE_ALIGN`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PFN_DOWN`
- 大写宏：`PFN_UP`
- 大写宏：`PHYS_ADDR_MAX`
- 大写宏：`RESERVE_MEM_KHO_NODE_COMPATIBLE`
- 大写宏：`SEV`
- 大写宏：`SMP_CACHE_BYTES`
- 大写宏：`SNP`
- 大写宏：`TDX`
- 大写宏：`WARN`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`APIs`
- 外部类型：`Accept`
- 外部类型：`Allocates`
- 外部类型：`Calculate`
- 外部类型：`Can`
- 外部类型：`Cannot`
- 外部类型：`Could`
- 外部类型：`Detect`
- 外部类型：`Does`
- 外部类型：`Failed`
- 外部类型：`Found`
- 外部类型：`Free`
- 外部类型：`Going`
- 外部类型：`If`
- 外部类型：`Intel`
- 外部类型：`Machine`
- 外部类型：`Memblock`
- 外部类型：`Once`
- 外部类型：`Otherwise`
- 外部类型：`Perform`
- 外部类型：`Public`
- 外部类型：`Reserve`
- 外部类型：`Retrieve`
- 外部类型：`Return`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`Some`
- 外部类型：`Special`
- 外部类型：`Split`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Try`
- 外部类型：`Update`
- 外部类型：`Virtual`
- 外部类型：`WARNs`
- 外部类型：`We`
- 外部类型：`memblock_flags`
- 外部类型：`memblock_region`
- 外部类型：`memblock_type`
- 外部类型：`page`
- 外部类型：`pg_data_t`
- 外部类型：`pglist_data`
- 外部类型：`phys_addr_t`
- 外部类型：`size_t`
- 外部类型：`zone`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d4c3bc5fdb` → 本草稿移入 `cases/defect/auto-linux-d4c3bc5fdb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
