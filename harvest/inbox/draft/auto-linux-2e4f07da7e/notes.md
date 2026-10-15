# auto-linux-2e4f07da7e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#786262be6048deab760f68c8acc2c85607165894](https://github.com/torvalds/linux/commit/786262be6048deab760f68c8acc2c85607165894) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 142 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #786262be6048deab760f68c8acc2c85607165894 (https://github.com/torvalds/linux/commit/786262be6048deab760f68c8acc2c85607165894)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1483；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 786262be6048deab760f68c8acc2c85607165894 Merge tag 'edac_updates_for_v7.3_rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/ras/ras :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -42,7 +42,8 @@
 
 #define GET_BITFIELD(v, lo, hi) (((v) & GENMASK_ULL(hi, lo)) >> (lo))
 
-#define NUM_IMC				2 /* Max memory controllers */
+/* Probing upper bound, not a hardware capability limit. */
+#define MAX_IMC_TO_PROBE		8
 #define NUM_CHANNELS			2 /* Max channels */
 #define NUM_DIMMS			2 /* Max DIMMs per channel */
 
@@ -122,6 +123,43 @@
 #define MEM_SLICE_HASH_MASK(v)		(GET_BITFIELD(v, 6, 19) << 6)
 #define MEM_SLICE_HASH_LSB_MASK_BIT(v)	GET_BITFIELD(v, 24, 26)
 
+/*
+ * A slice represents a portion of memory space participating in an
+ * interleave relationship within the memory hierarchy.
+ *
+ * It can represent in different levels such as:
+ *
+ *   - a pair of memory controllers
+ *   - a memory controller
+ *   - a memory channel
+ *   - a memory sub-channel / DIMM
+ *
+ * +--------+
+ * |        |
+ * | Zone 1 |
+ * |        |
+ * +--------+  +--------+
+ * |        |  |        |
+ * |        |  |        |
+ * | Zone 0 |  | Zone 0 |
+ * |        |  |        |
+ * |        |  |        |
+ * +--------+  +--------+
+ *
+ *  Slice L     Slice S
+ *
+ * Memory space is divided into:
+ *
+ *   - Zone 0 : Interleaved region
+ *   - Zone 1 : Non-interleaved region (upper part of the large slice).
+ */
+struct slice {
+	/* Slice address. */
+	u64 addr;
+	/* Slice that @addr belongs to. */
+	int id;
+};
+
 struct igen6_imc {
 	int mc;
 	struct mem_ctl_info *mci;
@@ -175,20 +213,18 @@ static struct res_config {
 	/* Set imc->dimm_{l_size,s_size,l_map}[chan]. */
 	void (*set_dimm_params)(struct igen6_imc *imc, int chan);
 	bool (*ibecc_available)(struct pci_dev *pdev);
-	/* Extract error address logged in IBECC */
-	u64 (*err_addr)(u64 ecclog);
 	/* Convert error address logged in IBECC to system physical address */
 	u64 (*err_addr_to_sys_addr)(u64 eaddr, int mc);
 	/* Convert error address logged in IBECC to integrated memory controller address */
 	u64 (*err_addr_to_imc_addr)(u64 eaddr, int mc);
 } *res_cfg;
 
 static struct igen6_pvt {
-	struct igen6_imc imc[NUM_IMC];
 	void __iomem *memss_pma_cr;
 	u64 ms_hash;
 	u64 ms_s_size;
 	int ms_l_map;
+	struct igen6_imc imc[];
 } *igen6_pvt;
 
 /* The top of low usable DRAM */
@@ -225,7 +261,8 @@ static char ecclog_buf[ECCLOG_POOL_SIZE];
 static struct irq_work ecclog_irq_work;
 static struct work_struct ecclog_work;
 
-/* Compute die IDs for Elkhart Lake with IBECC */
+/* SoC compute die IDs with IBECC capability. */
+/* Elkhart Lake */
 #define DID_EHL_SKU5	0x4514
 #define DID_EHL_SKU6	0x4528
 #define DID_EHL_SKU7	0x452a
@@ -238,22 +275,22 @@ static struct work_struct ecclog_work;
 #define DID_EHL_SKU14	0x4534
 #define DID_EHL_SKU15	0x4536
 
-/* Compute die IDs for ICL-NNPI with IBECC */
+/* ICL-NNPI */
 #define DID_ICL_SKU8	0x4581
 #define DID_ICL_SKU10	0x4585
 #define DID_ICL_SKU11	0x4589
 #define DID_ICL_SKU12	0x458d
 
-/* Compute die IDs for Tiger Lake with IBECC */
+/* Tiger Lake */
 #define DID_TGL_SKU	0x9a14
 
-/* Compute die IDs for Alder Lake with IBECC */
+/* Alder Lake */
 #define DID_ADL_SKU1	0x4601
 #define DID_ADL_SKU2	0x4602
 #define DID_ADL_SKU3	0x4621
 #define DID_ADL_SKU4	0x4641
 
-/* Compute die IDs for Alder Lake-N with IBECC */
+/* Alder Lake-N */
 #define DID_ADL_N_SKU1	0x4614
 #define DID_ADL_N_SKU2	0x4617
 #define DID_ADL_N_SKU3	0x461b
@@ -267,38 +304,38 @@ static struct work_struct ecclog_work;
 #define DID_ADL_N_SKU11	0x467c
 #define DID_ADL_N_SKU12	0x4632
 
-/* Compute die IDs for Arizona Beach with IBECC */
+/* Arizona Beach */
 #define DID_AZB_SKU1	0x4676
 
-/* Compute did IDs for Amston Lake with IBECC */
+/* Amston Lake */
 #define DID_ASL_SKU1	0x464a
 #define DID_ASL_SKU2	0x4646
 #define DID_ASL_SKU3	0x4652
 
-/* Compute die IDs for Raptor Lake-P with IBECC */
+/* Raptor Lake-P */
 #define DID_RPL_P_SKU1	0xa706
 #define DID_RPL_P_SKU2	0xa707
 #define DID_RPL_P_SKU3	0xa708
 #define DID_RPL_P_SKU4	0xa716
 #define DID_RPL_P_SKU5	0xa718
 
-/* Compute die IDs for Meteor Lake-PS with IBECC */
+/* Meteor Lake-PS */
 #define DID_MTL_PS_SKU1	0x7d
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bool`
- 外部函数：`dev_type`
- 外部函数：`device_initialize`
- 外部函数：`edac_dbg`
- 外部函数：`edac_get_dimm`
- 外部函数：`edac_mc_add_mc`
- 外部函数：`edac_mc_alloc`
- 外部函数：`edac_mc_del_mc`
- 外部函数：`edac_mc_free`
- 外部函数：`edac_printk`
- 外部函数：`err_addr`
- 外部函数：`err_addr_to_imc_addr`
- 外部函数：`err_addr_to_sys_addr`
- 外部函数：`errsts_clear`
- 外部函数：`field_get`
- 外部函数：`gen_pool_alloc`
- 外部函数：`igen6_printk`
- 外部函数：`ioremap`
- 外部函数：`iounmap`
- 外部函数：`irq_work_queue`
- 外部函数：`kasprintf`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`llist_add`
- 外部函数：`mem_type`
- 外部函数：`memset`
- 外部函数：`pci_dev`
- 外部函数：`pci_name`
- 外部函数：`pci_read_config_dword`
- 外部函数：`pdev`
- 外部函数：`put_device`
- 外部函数：`readl`
- 外部函数：`readq`
- 外部函数：`snprintf`
- 外部函数：`tgl_err_addr_to_imc_addr`
- 外部函数：`u`
- 外部函数：`u64`
- 外部函数：`void`
- 外部函数：`writeq`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BIT`
- 大写宏：`BIT_ULL`
- 大写宏：`CHANNEL_EHASH`
- 大写宏：`CHANNEL_HASH`
- 大写宏：`DEV_UNKNOWN`
- 大写宏：`DEV_X16`
- 大写宏：`DEV_X32`
- 大写宏：`DEV_X8`
- 大写宏：`DIMM`
- 大写宏：`DRAM`
- 大写宏：`ECC`
- 大写宏：`ECC_ERROR_LOG`
- 大写宏：`EDAC`
- 大写宏：`EDAC_FLAG_SECDED`
- 大写宏：`EDAC_MC_LAYER_CHANNEL`
- 大写宏：`EDAC_MC_LAYER_SLOT`
- 大写宏：`EDAC_OPSTATE_POLL`
- 大写宏：`EDAC_SECDED`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`GENMASK_ULL`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IBECC`
- 大写宏：`ICL`
- 大写宏：`INTEL`
- 大写宏：`IRQ`
- 大写宏：`KERN_ERR`
- 大写宏：`MAD_DIMM_CH`
- 大写宏：`MAD_INTER_CHANNEL`
- 大写宏：`MAD_INTRA_CH`
- 大写宏：`MEMSS_PMA_CR`
- 大写宏：`MEM_DDR3`
- 大写宏：`MEM_DDR4`
- 大写宏：`MEM_DDR5`
- 大写宏：`MEM_FLAG_DDR4`
- 大写宏：`MEM_FLAG_LPDDR4`
- 大写宏：`MEM_LPDDR3`
- 大写宏：`MEM_LPDDR4`
- 大写宏：`MEM_LPDDR5`
- 大写宏：`MEM_UNKNOWN`
- 大写宏：`MEM_WIO2`
- 大写宏：`MMIO`
- 大写宏：`NMI`
- 大写宏：`NNPI`
- 大写宏：`NULL`
- 大写宏：`PCI_VDEVICE`
- 大写宏：`TOLUD`
- 大写宏：`TOM`
- 大写宏：`TOUUD`
- 外部类型：`Address`
- 外部类型：`Alder`
- 外部类型：`Amston`
- 外部类型：`Arizona`
- 外部类型：`Arrow`
- 外部类型：`Band`
- 外部类型：`Beach`
- 外部类型：`Channel`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Compute`
- 外部类型：`Convert`
- 外部类型：`DIMMs`
- 外部类型：`Decode`
- 外部类型：`Delay`
- 外部类型：`Done`
- 外部类型：`Elkhart`
- 外部类型：`Extract`
- 外部类型：`Failed`
- 外部类型：`Get`
- 外部类型：`Host`
- 外部类型：`IDs`
- 外部类型：`In`
- 外部类型：`Intel_client_SoC`
- 外部类型：`Lake`
- 外部类型：`MCs`
- 外部类型：`Max`
- 外部类型：`Memory`
- 外部类型：`Meteor`
- 外部类型：`MiB`
- 外部类型：`Neither`
- 外部类型：`Nothing`
- 外部类型：`Nova`
- 外部类型：`Panther`
- 外部类型：`Quirk`
- 外部类型：`Raptor`
- 外部类型：`Remove`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`Skip`
- 外部类型：`SoCs`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Tiger`
- 外部类型：`To`
- 外部类型：`Top`
- 外部类型：`Wildcat`
- 外部类型：`dev_type`
- 外部类型：`device`
- 外部类型：`dimm_info`
- 外部类型：`edac_mc_layer`
- 外部类型：`irq_work`
- 外部类型：`kernel_ulong_t`
- 外部类型：`llist_node`
- 外部类型：`mem_ctl_info`
- 外部类型：`mem_type`
- 外部类型：`pci_dev`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-2e4f07da7e` → 本草稿移入 `cases/defect/auto-linux-2e4f07da7e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
