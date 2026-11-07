# auto-linux-a4fbde7ba0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a7963b12ca3e764e6ad3b15d1373430eb02492ee](https://github.com/torvalds/linux/commit/a7963b12ca3e764e6ad3b15d1373430eb02492ee) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 258 |
| 编译错误数（gcc syntax-only） | 138（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a7963b12ca3e764e6ad3b15d1373430eb02492ee (https://github.com/torvalds/linux/commit/a7963b12ca3e764e6ad3b15d1373430eb02492ee)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1963；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a7963b12ca3e764e6ad3b15d1373430eb02492ee Merge tag 'iommu-fixes-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/iommu/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -432,8 +432,41 @@ static void iommu_enable(struct amd_iommu *iommu)
 	iommu_feature_enable(iommu, CONTROL_IOMMU_EN);
 }
 
+/*
+ * Program or clear the per-IOMMU PerfOpt enable bit.
+ * Gate on this IOMMU's own EFR, not the global AND-mask.
+ * Caller serializes access using iommu->lock or during init/suspend/resume.
+ */
+int amd_iommu_perfopt_set(struct amd_iommu *iommu, bool enable)
+{
+	u32 old, val, readback;
+
+	if (!(readq(iommu->mmio_base + MMIO_EXT_FEATURES) & FEATURE_PERF_OPT))
+		return enable ? -ENODEV : 0;
+
+	old = readl(iommu->mmio_base + MMIO_PERF_OPT_OFFSET);
+	if (old == U32_MAX)
+		goto fail;
+
+	val = enable ? old | PERF_OPT_EN : old & ~PERF_OPT_EN;
+	if (val != old)
+		writel(val, iommu->mmio_base + MMIO_PERF_OPT_OFFSET);
+	readback = readl(iommu->mmio_base + MMIO_PERF_OPT_OFFSET);
+	if (readback == U32_MAX ||
+	    (readback & PERF_OPT_EN) != (val & PERF_OPT_EN))
+		goto fail;
+
+	iommu->perfopt_enabled = enable;
+	return 0;
+fail:
+	pr_err("IOMMU%d: failed to set PerfOpt to %d\n", iommu->index, enable);
+	return -EIO;
+}
+
 static void iommu_disable(struct amd_iommu *iommu)
 {
+	bool perfopt_enabled = iommu->perfopt_enabled;
+
 	if (!iommu->mmio_base)
 		return;
 
@@ -457,6 +490,10 @@ static void iommu_disable(struct amd_iommu *iommu)
 
 	/* Clear IRTE cache disabling bit */
 	iommu_feature_disable(iommu, CONTROL_IRTCACHEDIS);
+
+	/* Clear hardware while preserving the activation state for resume. */
+	amd_iommu_perfopt_set(iommu, false);
+	iommu->perfopt_enabled = perfopt_enabled;
 }
 
 /*
@@ -1888,6 +1925,7 @@ static int __init init_iommu_one(struct amd_iommu *iommu, struct ivhd_header *h,
 				 struct acpi_table_header *ivrs_base)
 {
 	struct amd_iommu_pci_seg *pci_seg;
+	int ret;
 
 	pci_seg = get_pci_segment(h->pci_seg, ivrs_base);
 	if (pci_seg == NULL)
@@ -1960,8 +1998,10 @@ static int __init init_iommu_one(struct amd_iommu *iommu, struct ivhd_header *h,
 	if (!iommu->mmio_base)
 		return -ENOMEM;
 
-	if (amd_iommu_perfopt_clear(iommu))
-		pr_err("IOMMU%d: failed to clear PerfOpt\n", iommu->index);
+	/* A crash kernel may reuse device tables with PerfOpt still enabled. */
+	ret = amd_iommu_perfopt_set(iommu, false);
+	if (ret)
+		return ret;
 
 	return init_iommu_from_acpi(iommu, h);
 }
@@ -2331,6 +2371,9 @@ static void print_iommu_info(void)
 		if (check_feature2(FEATURE_SEVSNPIO_SUP))
 			pr_cont(" SEV-TIO");
 
+		if (check_feature(FEATURE_PERF_OPT))
+			pr_cont(" PerfOpt");
+
 		pr_cont("\n");
 	}
 
@@ -2733,6 +2776,15 @@ static int __init init_unity_map_range(struct ivmd_header *m,
 	return 0;
 }
 
+static bool ivrs_block_type_ivmd(u8 type)
+{
+	if (type == ACPI_IVMD_TYPE ||
+	    type == ACPI_IVMD_TYPE_ALL ||
+	    type == ACPI_IVMD_TYPE_RANGE)
+		return true;
+	return false;
+}
+
 /* iterates over all memory definitions we find in the ACPI table */
 static int __init init_memory_definitions(struct acpi_table_header *table)
 {
@@ -2744,10 +2796,11 @@ static int __init init_memory_definitions(struct acpi_table_header *table)
 
 	while (p < end) {
 		m = (struct ivmd_header *)p;
+		p += m->length;
+		if (!ivrs_block_type_ivmd(m->type))
+			continue;
 		if (m->flags & (IVMD_FLAG_UNITY_MAP | IVMD_FLAG_EXCL_RANGE))
 			init_unity_map_range(m, table);
-
-		p += m->length;
 	}
 
 	return 0;
@@ -3067,46 +3120,10 @@ static void enable_iommus_vapic(void)
 #endif
 }
 
-static int clear_perfopt_all(void)
-{
-	struct amd_iommu *iommu;
-	int err, ret = 0;
-
-	for_each_iommu(iommu) {
-		err = amd_iommu_perfopt_clear(iommu);
-		if (err)
-			ret = err;
-	}
-
-	return ret;
-}
-
-static int restore_perfopt_all(void)
-{
-	struct amd_iommu *iommu;
-	int err, ret = 0;
-
-	for_each_iommu(iommu) {
-		err = amd_iommu_perfopt_restore(iommu);
-		if (err)
-			ret = err;
-	}
-
-	return ret;
-}
-
 static void disable_iommus(void)
 {
 	struct amd_iommu *iommu;
 
-	/*
-	 * PerfOpt is an optional performance bit, so a failure to clear it must
-	 * not skip the mandatory disable below. This also runs from the void
-	 * amd_
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`amd_iommu_apply_ivrs_quirks`
- 外部函数：`amd_iommu_disable`
- 外部函数：`amd_iommu_enable_ppr_log`
- 外部函数：`amd_iommu_flush_all_caches`
- 外部函数：`amd_iommu_gt_ppr_supported`
- 外部函数：`amd_iommu_perfopt_clear`
- 外部函数：`amd_iommu_perfopt_restore`
- 外部函数：`amd_iommu_set_rlookup_table`
- 外部函数：`cc_platform_clear`
- 外部函数：`check_feature`
- 外部函数：`check_feature2`
- 外部函数：`d`
- 外部函数：`dev_to_node`
- 外部函数：`get_dev_table`
- 外部函数：`here`
- 外部函数：`init_irq_alloc_info`
- 外部函数：`init_llist_head`
- 外部函数：`iommu_alloc_pages_sz`
- 外部函数：`iommu_free_pages`
- 外部函数：`iommu_snp_enable`
- 外部函数：`iommu_virt_to_phys`
- 外部函数：`irq_domain_alloc_irqs`
- 外部函数：`irq_domain_alloc_named_fwnode`
- 外部函数：`irq_domain_create_hierarchy`
- 外部函数：`irq_domain_free_fwnode`
- 外部函数：`irq_domain_free_irqs`
- 外部函数：`irq_domain_remove`
- 外部函数：`is_kdump_kernel`
- 外部函数：`kfree`
- 外部函数：`kvfree`
- 外部函数：`kvmalloc_objs`
- 外部函数：`kvzalloc_objs`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`max`
- 外部函数：`memcpy`
- 外部函数：`memcpy_toio`
- 外部函数：`memory`
- 外部函数：`memunmap`
- 外部函数：`mode`
- 外部函数：`pci_disable_msi`
- 外部函数：`pci_enable_msi`
- 外部函数：`pr_cont`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`pr_warn`
- 外部函数：`range`
- 外部函数：`ranges`
- 外部函数：`readl`
- 外部函数：`readq`
- 外部函数：`request_threaded_irq`
- 外部函数：`roundup_pow_of_two`
- 外部函数：`snprintf`
- 外部函数：`sprintf`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`struct_group`
- 外部函数：`udelay`
- 外部函数：`writel`
- 外部函数：`writeq`
- 大写宏：`ACPI`
- 大写宏：`ACPIHID_HID_LEN`
- 大写宏：`ACPIHID_UID_LEN`
- 大写宏：`ACPI_HID`
- 大写宏：`AMD`
- 大写宏：`AMD_IOMMU_GUEST_IR_GA`
- 大写宏：`AMD_IOMMU_GUEST_IR_LEGACY_GA`
- 大写宏：`AMD_IOMMU_GUEST_IR_VAPIC`
- 大写宏：`APIC`
- 大写宏：`BDF`
- 大写宏：`BIT_ULL`
- 大写宏：`BUG_ON`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CC_ATTR_HOST_SEV_SNP`
- 大写宏：`CONFIG_IRQ_REMAP`
- 大写宏：`CONTROL_CMDBUF_EN`
- 大写宏：`CONTROL_COHERENT_EN`
- 大写宏：`CONTROL_EPH_EN`
- 大写宏：`CONTROL_EVT_INT_EN`
- 大写宏：`CONTROL_EVT_LOG_EN`
- 大写宏：`CONTROL_GAINT_EN`
- 大写宏：`CONTROL_GALOG_EN`
- 大写宏：`CONTROL_GAM_EN`
- 大写宏：`CONTROL_GA_EN`
- 大写宏：`CONTROL_GCR3TRPMODE`
- 大写宏：`CONTROL_GT_EN`
- 大写宏：`CONTROL_HT_TUN_EN`
- 大写宏：`CONTROL_INTCAPXT_EN`
- 大写宏：`CONTROL_INV_TIMEOUT`
- 大写宏：`CONTROL_IOMMU_EN`
- 大写宏：`CONTROL_IRTCACHEDIS`
- 大写宏：`CONTROL_ISOC_EN`
- 大写宏：`CONTROL_NUM_INT_REMAP_MODE`
- 大写宏：`CONTROL_NUM_INT_REMAP_MODE_2K`
- 大写宏：`CONTROL_NUM_INT_REMAP_MODE_MASK`
- 大写宏：`CONTROL_PASSPW_EN`
- 大写宏：`CONTROL_RESPASSPW_EN`
- 大写宏：`CONTROL_SNPAVIC_EN`
- 大写宏：`CONTROL_XT_EN`
- 大写宏：`CTRL_INV_TO_1S`
- 大写宏：`CTRL_INV_TO_MASK`
- 大写宏：`DEV_ACPI_HID`
- 大写宏：`DEV_ALIAS`
- 大写宏：`DEV_ALIAS_RANGE`
- 大写宏：`DEV_ALL`
- 大写宏：`DEV_ENTRY_EINT_PASS`
- 大写宏：`DEV_ENTRY_INIT_PASS`
- 大写宏：`DEV_ENTRY_LINT0_PASS`
- 大写宏：`DEV_ENTRY_LINT1_PASS`
- 大写宏：`DEV_ENTRY_NMI_PASS`
- 大写宏：`DEV_ENTRY_SYSMGT1`
- 大写宏：`DEV_ENTRY_SYSMGT2`
- 大写宏：`DEV_EXT_SELECT`
- 大写宏：`DEV_EXT_SELECT_RANGE`
- 大写宏：`DEV_RANGE_END`
- 大写宏：`DEV_SELECT`
- 大写宏：`DEV_SELECT_RANGE_START`
- 大写宏：`DEV_SPECIAL`
- 大写宏：`DEV_TABLE_ENTRY_SIZE`
- 大写宏：`DMA32`
- 大写宏：`DTE`
- 大写宏：`DTE_DATA1_SYSMGT_MASK`
- 大写宏：`DTE_FLAG_IW`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`ENXIO`
- 大写宏：`EVTLOG_LEN_MASK_DEF`
- 大写宏：`EVTLOG_LEN_MASK_MAX`
- 大写宏：`EVTLOG_SIZE_DEF`
- 大写宏：`FEATURE_EPHSUP`
- 大写宏：`FEATURE_GCR3TRPMODE`
- 大写宏：`FEATURE_GT`
- 大写宏：`FEATURE_NUM_INT_REMAP_SUP_2K`
- 大写宏：`FEATURE_SEVSNPIO_SUP`
- 大写宏：`FIELD_GET`
- 大写宏：`FW_BUG`
- 大写宏：`GAM`
- 大写宏：`GA_LOG_SIZE_512`
- 大写宏：`GFP_DMA32`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HID`
- 大写宏：`HPET`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`IOAPIC`
- 大写宏：`IOMMU`
- 大写宏：`IOTLB`
- 大写宏：`IRQF_ONESHOT`
- 大写宏：`IRQ_POSTING_CAP`
- 大写宏：`IRQ_REMAP_X2APIC_MODE`
- 大写宏：`IRT`
- 大写宏：`IRTE`
- 大写宏：`IVHD`
- 大写宏：`IVMD`
- 大写宏：`IVMD_TYPE_ALL`
- 大写宏：`IVMD_TYPE_RANGE`
- 大写宏：`MMIO`
- 大写宏：`MMIO_CMD_BUF_OFFSET`
- 大写宏：`MMIO_CMD_HEAD_OFFSET`
- 大写宏：`MMIO_CMD_SIZE_512`
- 大写宏：`MMIO_CMD_TAIL_OFFSET`
- 大写宏：`MMIO_CONTROL_OFFSET`
- 大写宏：`MMIO_DEV_TABLE_OFFSET`
- 大写宏：`MMIO_EVT_BUF_OFFSET`
- 大写宏：`MMIO_EVT_HEAD_OFFSET`
- 大写宏：`MMIO_EVT_TAIL_OFFSET`
- 大写宏：`MMIO_EXT_FEATURES`
- 大写宏：`MMIO_GA_HEAD_OFFSET`
- 大写宏：`MMIO_GA_LOG_BASE_OFFSET`
- 大写宏：`MMIO_GA_LOG_TAIL_OFFSET`
- 大写宏：`MMIO_GA_TAIL_OFFSET`
- 大写宏：`MMIO_INTCAPXT_EVT_OFFSET`
- 大写宏：`MMIO_INTCAPXT_GALOG_OFFSET`
- 大写宏：`MMIO_INTCAPXT_PPR_OFFSET`
- 大写宏：`MMIO_STATUS_GALOG_RUN_MASK`
- 大写宏：`MMIO_STATUS_OFFSET`
- 大写宏：`MMIO_STATUS_TIMEOUT`
- 大写宏：`MSI`
- 大写宏：`NULL`
- 大写宏：`PAGE_ALIGN`
- 大写宏：`PCI`
- 大写宏：`PCI_BUS_NUM`
- 大写宏：`PCI_FUNC`
- 大写宏：`PCI_SEG_DEVID_TO_SBDF`
- 大写宏：`PCI_SLOT`
- 大写宏：`PERF_OPT_EN`
- 大写宏：`PPR`
- 大写宏：`SEV`
- 大写宏：`SNP`
- 大写宏：`SNPAVIC`
- 大写宏：`SZ_4K`
- 大写宏：`TIO`
- 大写宏：`UID`
- 大写宏：`UNKNOWN`
- 大写宏：`WARN_ON`
- 大写宏：`X86_IRQ_ALLOC_TYPE_AMDVI`
- 外部类型：`Apply`
- 外部类型：`BIOSes`
- 外部类型：`Bus`
- 外部类型：`Clear`
- 外部类型：`Command`
- 外部类型：`Dev`
- 外部类型：`Disable`
- 外部类型：`Done`
- 外部类型：`Enable`
- 外部类型：`Enabling`
- 外部类型：`Enhanced`
- 外部类型：`Event`
- 外部类型：`Evt`
- 外部类型：`Exact`
- 外部类型：`Failed`
- 外部类型：`First`
- 外部类型：`Following`
- 外部类型：`For`
- 外部类型：`Func`
- 外部类型：`GALOGRun`
- 外部类型：`GALog`
- 外部类型：`Handling`
- 外部类型：`IOMMUs`
- 外部类型：`IRTCacheDis`
- 外部类型：`IVMD_TYPEi`
- 外部类型：`If`
- 外部类型：`Interrupt`
- 外部类型：`Invalid`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Log`
- 外部类型：`Need`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Page`
- 外部类型：`Parse`
- 外部类型：`PerfOpt`
- 外部类型：`Peripheral`
- 外部类型：`Request`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Treat`
- 外部类型：`Unsupported`
- 外部类型：`Upon`
- 外部类型：`Use`
- 外部类型：`Vi`
- 外部类型：`Virtual`
- 外部类型：`acpi_table_header`
- 外部类型：`acpihid_map_entry`
- 外部类型：`amd_iommu`
- 外部类型：`amd_iommu_pci_seg`
- 外部类型：`dev_table_entry`
- 外部类型：`devid_map`
- 外部类型：`fwnode_handle`
- 外部类型：`gfp_t`
- 外部类型：`irq_alloc_info`
- 外部类型：`irq_domain`
- 外部类型：`irq_handler_t`
- 外部类型：`ivhd_dte_flags`
- 外部类型：`list_head`
- 外部类型：`unity_map_entry`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a4fbde7ba0` → 本草稿移入 `cases/defect/auto-linux-a4fbde7ba0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
