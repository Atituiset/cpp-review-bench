# auto-linux-92cbde4d67

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#067c5c24cef23d913b67eaaf71ae67e7169a6620](https://github.com/torvalds/linux/commit/067c5c24cef23d913b67eaaf71ae67e7169a6620) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 169 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #067c5c24cef23d913b67eaaf71ae67e7169a6620 (https://github.com/torvalds/linux/commit/067c5c24cef23d913b67eaaf71ae67e7169a6620)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1858；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 067c5c24cef23d913b67eaaf71ae67e7169a6620 Merge tag 'page_alloc-for-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/mm/page_alloc :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1439,7 +1439,6 @@ struct zoneref {
  *
  * zonelist_zone()	- Return the struct zone * for an entry in _zonerefs
  * zonelist_zone_idx()	- Return the index of the zone for an entry
- * zonelist_node_idx()	- Return the index of the node for an entry
  */
 struct zonelist {
 	struct zoneref _zonerefs[MAX_ZONES_PER_ZONELIST + 1];
@@ -1855,19 +1854,15 @@ extern struct zone *next_zone(struct zone *zone);
 
 static inline struct zone *zonelist_zone(struct zoneref *zoneref)
 {
-	return zoneref->zone;
+	/* Memory hotplug rewrites zonelists in place. */
+	return READ_ONCE(zoneref->zone);
 }
 
 static inline int zonelist_zone_idx(const struct zoneref *zoneref)
 {
 	return zoneref->zone_idx;
 }
 
-static inline int zonelist_node_idx(const struct zoneref *zoneref)
-{
-	return zone_to_nid(zoneref->zone);
-}
-
 struct zoneref *__next_zones_zonelist(struct zoneref *z,
 					enum zone_type highest_zoneidx,
 					const nodemask_t *nodes);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__next_zones_zonelist`
- 外部函数：`_wmark_pages`
- 外部函数：`absent_pages`
- 外部函数：`again`
- 外部函数：`alloc_contig_range`
- 外部函数：`as`
- 外部函数：`buddy`
- 外部函数：`cases`
- 外部函数：`defined`
- 外部函数：`done`
- 外部函数：`example`
- 外部函数：`fail`
- 外部函数：`flags`
- 外部函数：`get_online_mems`
- 外部函数：`has_unmovable_pages`
- 外部函数：`hotplug`
- 外部函数：`hotunplug`
- 外部函数：`hwpoison_filter`
- 外部函数：`mappings`
- 外部函数：`mem_cgroup_lruvec`
- 外部函数：`memory`
- 外部函数：`memory_failure`
- 外部函数：`pfn_to_online_page`
- 外部函数：`pfn_to_page`
- 外部函数：`pgdat_resize_lock`
- 外部函数：`pgdat_resize_unlock`
- 外部函数：`pinned`
- 外部函数：`prepare_scan_control`
- 外部函数：`zone_to_nid`
- 大写宏：`CACHELINE_PADDING`
- 大写宏：`CMA`
- 大写宏：`CONFIG_CMA`
- 大写宏：`CONFIG_COMPACTION`
- 大写宏：`CONFIG_DEFERRED_STRUCT_PAGE_INIT`
- 大写宏：`CONFIG_FLATMEM`
- 大写宏：`CONFIG_HIGHMEM`
- 大写宏：`CONFIG_LRU_GEN`
- 大写宏：`CONFIG_LRU_GEN_WALKS_MMU`
- 大写宏：`CONFIG_MEMCG`
- 大写宏：`CONFIG_MEMORY_FAILURE`
- 大写宏：`CONFIG_MEMORY_HOTPLUG`
- 大写宏：`CONFIG_MEMORY_ISOLATION`
- 大写宏：`CONFIG_NUMA`
- 大写宏：`CONFIG_NUMA_BALANCING`
- 大写宏：`CONFIG_PAGE_EXTENSION`
- 大写宏：`CONFIG_SMP`
- 大写宏：`CONFIG_SPARSEMEM`
- 大写宏：`CONFIG_UNACCEPTED_MEMORY`
- 大写宏：`CONFIG_VMEMMAP_OPTIMIZATION`
- 大写宏：`CONFIG_ZONE_DEVICE`
- 大写宏：`CONFIG_ZONE_DMA`
- 大写宏：`CONFIG_ZONE_DMA32`
- 大写宏：`DMA`
- 大写宏：`ENABLED`
- 大写宏：`LRU`
- 大写宏：`MAX_NR_ZONES`
- 大写宏：`MAX_NUMNODES`
- 大写宏：`MAX_ZONELISTS`
- 大写宏：`MEMCG`
- 大写宏：`MIGRATE_CMA`
- 大写宏：`MIGRATE_PCPTYPES`
- 大写宏：`MOVABLE`
- 大写宏：`NOTE`
- 大写宏：`NR_LRU_LISTS`
- 大写宏：`NR_MM_STATS`
- 大写宏：`NR_VMSCAN_THROTTLE`
- 大写宏：`NR_VM_NODE_STAT_ITEMS`
- 大写宏：`NR_VM_ZONE_STAT_ITEMS`
- 大写宏：`NR_WMARK`
- 大写宏：`OOM`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PFN`
- 大写宏：`SGX`
- 大写宏：`SPARSEMEM`
- 大写宏：`THIS`
- 大写宏：`THP`
- 大写宏：`UNUSED`
- 大写宏：`XEN`
- 大写宏：`ZERO_PAGE`
- 大写宏：`ZONE_DEVICE`
- 大写宏：`ZONE_DMA`
- 大写宏：`ZONE_DMA32`
- 大写宏：`ZONE_HIGHMEM`
- 大写宏：`ZONE_MOVABLE`
- 大写宏：`ZONE_NORMAL`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`And`
- 外部类型：`Any`
- 外部类型：`As`
- 外部类型：`Bloom`
- 外部类型：`But`
- 外部类型：`Cases`
- 外部类型：`Fields`
- 外部类型：`Flags`
- 外部类型：`Generally`
- 外部类型：`HardwareCorrupted`
- 外部类型：`Here`
- 外部类型：`However`
- 外部类型：`Hyper`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Lists`
- 外部类型：`Locking`
- 外部类型：`Low`
- 外部类型：`Main`
- 外部类型：`Memory`
- 外部类型：`Must`
- 外部类型：`Nests`
- 外部类型：`Non`
- 外部类型：`Normal`
- 外部类型：`Not`
- 外部类型：`Notable`
- 外部类型：`Note`
- 外部类型：`Number`
- 外部类型：`On`
- 外部类型：`PG_hwpoison`
- 外部类型：`PG_migrate_skip`
- 外部类型：`PG_offline`
- 外部类型：`Pages`
- 外部类型：`Per`
- 外部类型：`Pinned`
- 外部类型：`Primarily`
- 外部类型：`Protected`
- 外部类型：`Protects`
- 外部类型：`Read`
- 外部类型：`Recovery`
- 外部类型：`Refaults`
- 外部类型：`Return`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`So`
- 外部类型：`Some`
- 外部类型：`Such`
- 外部类型：`The`
- 外部类型：`Therefore`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Unmovable`
- 外部类型：`Updated`
- 外部类型：`Use`
- 外部类型：`Various`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Write`
- 外部类型：`Zone`
- 外部类型：`atomic_long_t`
- 外部类型：`atomic_t`
- 外部类型：`free_area`
- 外部类型：`hlist_nulls_head`
- 外部类型：`hlist_nulls_node`
- 外部类型：`list_head`
- 外部类型：`llist_head`
- 外部类型：`lruvec_flags`
- 外部类型：`mem_section`
- 外部类型：`memory_tier`
- 外部类型：`mutex`
- 外部类型：`nodemask_t`
- 外部类型：`page`
- 外部类型：`page_ext`
- 外部类型：`pg_data_t`
- 外部类型：`seqlock_t`
- 外部类型：`spinlock_t`
- 外部类型：`task_struct`
- 外部类型：`wait_queue_head_t`
- 外部类型：`zoneref`
- 外部类型：`zswap_lruvec_state`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-92cbde4d67` → 本草稿移入 `cases/defect/auto-linux-92cbde4d67/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
