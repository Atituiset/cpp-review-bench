# auto-linux-617800d642

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#6addb4f385570ebc11c4eb499a4f1c149f313e84](https://github.com/torvalds/linux/commit/6addb4f385570ebc11c4eb499a4f1c149f313e84) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 156 |
| 编译错误数（gcc syntax-only） | 199（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6addb4f385570ebc11c4eb499a4f1c149f313e84 (https://github.com/torvalds/linux/commit/6addb4f385570ebc11c4eb499a4f1c149f313e84)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1537；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6addb4f385570ebc11c4eb499a4f1c149f313e84 Merge tag 'edac_urgent_for_v7.3_rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/ras/ras :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -453,15 +453,6 @@ static int altr_sdram_probe(struct platform_device *pdev)
 	return res;
 }
 
-static void altr_sdram_remove(struct platform_device *pdev)
-{
-	struct mem_ctl_info *mci = platform_get_drvdata(pdev);
-
-	edac_mc_del_mc(&pdev->dev);
-	edac_mc_free(mci);
-	platform_set_drvdata(pdev, NULL);
-}
-
 /*
  * If you want to suspend, need to disable EDAC by removing it
  * from the device tree or defconfig.
@@ -481,13 +472,13 @@ static const struct dev_pm_ops altr_sdram_pm_ops = {
 
 static struct platform_driver altr_sdram_edac_driver = {
 	.probe = altr_sdram_probe,
-	.remove = altr_sdram_remove,
 	.driver = {
 		.name = "altr_sdram_edac",
 #ifdef CONFIG_PM
 		.pm = &altr_sdram_pm_ops,
 #endif
 		.of_match_table = altr_sdram_ctrl_of_match,
+		.suppress_bind_attrs = true,
 	},
 };
 
@@ -517,6 +508,7 @@ static struct platform_driver altr_edac_driver = {
 	.driver = {
 		.name = "socfpga_ecc_manager",
 		.of_match_table = altr_edac_of_match,
+		.suppress_bind_attrs = true,
 	},
 };
 module_platform_driver(altr_edac_driver);
@@ -803,22 +795,12 @@ static int altr_edac_device_probe(struct platform_device *pdev)
 	return res;
 }
 
-static void altr_edac_device_remove(struct platform_device *pdev)
-{
-	struct edac_device_ctl_info *dci = platform_get_drvdata(pdev);
-	struct altr_edac_device_dev *drvdata = dci->pvt_info;
-
-	debugfs_remove_recursive(drvdata->debugfs_dir);
-	edac_device_del_device(&pdev->dev);
-	edac_device_free_ctl_info(dci);
-}
-
 static struct platform_driver altr_edac_device_driver = {
 	.probe =  altr_edac_device_probe,
-	.remove = altr_edac_device_remove,
 	.driver = {
 		.name = "altr_edac_device",
 		.of_match_table = altr_edac_device_of_match,
+		.suppress_bind_attrs = true,
 	},
 };
 module_platform_driver(altr_edac_device_driver);
@@ -944,7 +926,7 @@ static int __maybe_unused altr_init_memory_port(void __iomem *ioaddr, int port)
 	return ret;
 }
 
-static __init int __maybe_unused
+static int __maybe_unused
 altr_init_a10_ecc_block(struct device_node *np, u32 irq_mask,
 			u32 ecc_ctrl_en_mask, bool dual_port)
 {
@@ -1019,7 +1001,7 @@ altr_init_a10_ecc_block(struct device_node *np, u32 irq_mask,
 
 static int validate_parent_available(struct device_node *np);
 static const struct of_device_id altr_edac_a10_device_of_match[];
-static int __init __maybe_unused altr_init_a10_ecc_device_type(char *compat)
+static int __maybe_unused altr_init_a10_ecc_device_type(char *compat)
 {
 	int irq;
 	struct device_node *child, *np;
@@ -1348,7 +1330,7 @@ static const struct edac_device_prv_data a10_l2ecc_data = {
 
 #ifdef CONFIG_EDAC_ALTERA_ETHERNET
 
-static int __init socfpga_init_ethernet_ecc(struct altr_edac_device_dev *dev)
+static int socfpga_init_ethernet_ecc(struct altr_edac_device_dev *dev)
 {
 	int ret;
 
@@ -1378,7 +1360,7 @@ static const struct edac_device_prv_data a10_enetecc_data = {
 
 #ifdef CONFIG_EDAC_ALTERA_NAND
 
-static int __init socfpga_init_nand_ecc(struct altr_edac_device_dev *device)
+static int socfpga_init_nand_ecc(struct altr_edac_device_dev *device)
 {
 	int ret;
 
@@ -1408,7 +1390,7 @@ static const struct edac_device_prv_data a10_nandecc_data = {
 
 #ifdef CONFIG_EDAC_ALTERA_DMA
 
-static int __init socfpga_init_dma_ecc(struct altr_edac_device_dev *device)
+static int socfpga_init_dma_ecc(struct altr_edac_device_dev *device)
 {
 	int ret;
 
@@ -1438,7 +1420,7 @@ static const struct edac_device_prv_data a10_dmaecc_data = {
 
 #ifdef CONFIG_EDAC_ALTERA_USB
 
-static int __init socfpga_init_usb_ecc(struct altr_edac_device_dev *device)
+static int socfpga_init_usb_ecc(struct altr_edac_device_dev *device)
 {
 	int ret;
 
@@ -1468,7 +1450,7 @@ static const struct edac_device_prv_data a10_usbecc_data = {
 
 #ifdef CONFIG_EDAC_ALTERA_QSPI
 
-static int __init socfpga_init_qspi_ecc(struct altr_edac_device_dev *device)
+static int socfpga_init_qspi_ecc(struct altr_edac_device_dev *device)
 {
 	int ret;
 
@@ -1524,6 +1506,7 @@ static int altr_portb_setup(struct altr_edac_device_dev *device)
 	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`debugfs_remove_recursive`
- 外部函数：`dev_name`
- 外部函数：`device_get_match_data`
- 外部函数：`devm_ioremap`
- 外部函数：`devm_request_irq`
- 外部函数：`devm_request_mem_region`
- 外部函数：`devres_close_group`
- 外部函数：`devres_open_group`
- 外部函数：`devres_release_group`
- 外部函数：`devres_remove_group`
- 外部函数：`edac_dbg`
- 外部函数：`edac_debugfs_create_dir`
- 外部函数：`edac_debugfs_create_file`
- 外部函数：`edac_device_add_device`
- 外部函数：`edac_device_alloc_ctl_info`
- 外部函数：`edac_device_alloc_index`
- 外部函数：`edac_device_del_device`
- 外部函数：`edac_device_free_ctl_info`
- 外部函数：`edac_device_handle_ce`
- 外部函数：`edac_device_handle_ue`
- 外部函数：`edac_mc_add_mc`
- 外部函数：`edac_mc_alloc`
- 外部函数：`edac_mc_del_mc`
- 外部函数：`edac_mc_free`
- 外部函数：`edac_mc_handle_error`
- 外部函数：`edac_mc_printk`
- 外部函数：`edac_printk`
- 外部函数：`ioremap`
- 外部函数：`iounmap`
- 外部函数：`iowrite32`
- 外部函数：`irq_of_parse_and_map`
- 外部函数：`list_add`
- 外部函数：`module_platform_driver`
- 外部函数：`of_address_to_resource`
- 外部函数：`of_device_is_available`
- 外部函数：`of_device_is_compatible`
- 外部函数：`of_find_compatible_node`
- 外部函数：`of_machine_is_compatible`
- 外部函数：`of_match_node`
- 外部函数：`of_node_put`
- 外部函数：`of_parse_phandle`
- 外部函数：`of_property_read_u32_index`
- 外部函数：`panic`
- 外部函数：`platform_get_drvdata`
- 外部函数：`platform_get_irq`
- 外部函数：`platform_get_resource`
- 外部函数：`platform_set_drvdata`
- 外部函数：`readl`
- 外部函数：`regmap_read`
- 外部函数：`regmap_update_bits`
- 外部函数：`regmap_write`
- 外部函数：`release_mem_region`
- 外部函数：`request_mem_region`
- 外部函数：`resource_size`
- 外部函数：`setup`
- 外部函数：`syscon_regmap_lookup_by_phandle`
- 外部函数：`writel`
- 大写宏：`A10`
- 大写宏：`A10_DDR0_IRQ_MASK`
- 大写宏：`A10_INTMODE_OFST`
- 大写宏：`A10_INTMODE_SB_INT`
- 大写宏：`A10_SERRCNTREG_OFST`
- 大写宏：`A10_SYMAN_INTMASK_CLR`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`CONFIG_EDAC_ALTERA_DMA`
- 大写宏：`CONFIG_EDAC_ALTERA_ETHERNET`
- 大写宏：`CONFIG_EDAC_ALTERA_NAND`
- 大写宏：`CONFIG_EDAC_ALTERA_QSPI`
- 大写宏：`CONFIG_EDAC_ALTERA_USB`
- 大写宏：`CONFIG_EDAC_DEBUG`
- 大写宏：`CONFIG_PM`
- 大写宏：`DBERR`
- 大写宏：`DBIRQ`
- 大写宏：`DEV_X8`
- 大写宏：`EBUSY`
- 大写宏：`ECC`
- 大写宏：`ECC_DEVICE`
- 大写宏：`EDAC`
- 大写宏：`EDAC_FLAG_NONE`
- 大写宏：`EDAC_FLAG_SECDED`
- 大写宏：`EDAC_MC`
- 大写宏：`EDAC_MC_LAYER_CHANNEL`
- 大写宏：`EDAC_MC_LAYER_CHIP_SELECT`
- 大写宏：`EDAC_SECDED`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HW_EVENT_ERR_CORRECTED`
- 大写宏：`IORESOURCE_MEM`
- 大写宏：`IRQ`
- 大写宏：`IRQF_SHARED`
- 大写宏：`IRQF_TRIGGER_HIGH`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IRQ_NONE`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`KERN_ERR`
- 大写宏：`KERN_WARNING`
- 大写宏：`MEM_DDR3`
- 大写宏：`MEM_FLAG_DDR3`
- 大写宏：`NULL`
- 大写宏：`PAGE_MASK`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`S10`
- 大写宏：`SBERR`
- 大写宏：`SBIRQ`
- 大写宏：`SCRUB_SW_SRC`
- 大写宏：`SDMMC`
- 大写宏：`SDRAM`
- 大写宏：`S_IWUSR`
- 大写宏：`WARN_ON`
- 外部类型：`Altera`
- 外部类型：`Arria10`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Create`
- 外部类型：`Ensure`
- 外部类型：`Error`
- 外部类型：`Failed`
- 外部类型：`Get`
- 外部类型：`Grab`
- 外部类型：`IRQs`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Infrastructure`
- 外部类型：`Initialize`
- 外部类型：`Interrupt`
- 外部类型：`Manager`
- 外部类型：`No`
- 外部类型：`Only`
- 外部类型：`PortA`
- 外部类型：`PortB`
- 外部类型：`Probe`
- 外部类型：`SError`
- 外部类型：`Setup`
- 外部类型：`Toggle`
- 外部类型：`Unable`
- 外部类型：`Uncorrectable`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`Using`
- 外部类型：`Validate`
- 外部类型：`altr_edac_device_dev`
- 外部类型：`altr_sdram_mc_data`
- 外部类型：`altr_sdram_prv_data`
- 外部类型：`device_node`
- 外部类型：`dimm_info`
- 外部类型：`edac_device_ctl_info`
- 外部类型：`edac_device_prv_data`
- 外部类型：`edac_mc_layer`
- 外部类型：`irqreturn_t`
- 外部类型：`mem_ctl_info`
- 外部类型：`of_device_id`
- 外部类型：`platform_device`
- 外部类型：`platform_driver`
- 外部类型：`regmap`
- 外部类型：`resource`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-617800d642` → 本草稿移入 `cases/defect/auto-linux-617800d642/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
