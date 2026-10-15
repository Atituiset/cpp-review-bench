# auto-linux-6a9f80fb4c

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
| 外部依赖数（dep_count） | 141 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #786262be6048deab760f68c8acc2c85607165894 (https://github.com/torvalds/linux/commit/786262be6048deab760f68c8acc2c85607165894)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1583；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 786262be6048deab760f68c8acc2c85607165894 Merge tag 'edac_updates_for_v7.3_rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/ras/ras :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1507,6 +1507,7 @@ static int altr_portb_setup(struct altr_edac_device_dev *device)
 	int edac_idx, rc;
 	struct device_node *np;
 	const struct edac_device_prv_data *prv = &a10_sdmmceccb_data;
+	bool is_s10 = device->edac->is_s10;
 
 	rc = altr_check_ecc_deps(device);
 	if (rc)
@@ -1548,15 +1549,14 @@ static int altr_portb_setup(struct altr_edac_device_dev *device)
 
 	/*
 	 * Update the PortB IRQs - A10 has 4, S10 has 2, Index accordingly
-	 *
-	 * FIXME: Instead of ifdefs with different architectures the driver
-	 *        should properly use compatibles.
 	 */
-#ifdef CONFIG_64BIT
-	altdev->sb_irq = irq_of_parse_and_map(np, 1);
-#else
-	altdev->sb_irq = irq_of_parse_and_map(np, 2);
-#endif
+
+	/* Using compatibles to determine the IRQ Index */
+	if (is_s10)
+		altdev->sb_irq = irq_of_parse_and_map(np, 1);
+	else
+		altdev->sb_irq = irq_of_parse_and_map(np, 2);
+
 	if (!altdev->sb_irq) {
 		edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB SBIRQ alloc\n");
 		rc = -ENODEV;
@@ -1570,29 +1570,28 @@ static int altr_portb_setup(struct altr_edac_device_dev *device)
 		goto err_release_group_1;
 	}
 
-#ifdef CONFIG_64BIT
-	/* Use IRQ to determine SError origin instead of assigning IRQ */
-	rc = of_property_read_u32_index(np, "interrupts", 1, &altdev->db_irq);
-	if (rc) {
-		edac_printk(KERN_ERR, EDAC_DEVICE,
-			    "Error PortB DBIRQ alloc\n");
-		goto err_release_group_1;
-	}
-#else
-	altdev->db_irq = irq_of_parse_and_map(np, 3);
-	if (!altdev->db_irq) {
-		edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
-		rc = -ENODEV;
-		goto err_release_group_1;
-	}
-	rc = devm_request_irq(&altdev->ddev, altdev->db_irq,
-			      prv->ecc_irq_handler, IRQF_TRIGGER_HIGH,
-			      ecc_name, altdev);
-	if (rc) {
-		edac_printk(KERN_ERR, EDAC_DEVICE, "PortB DBERR IRQ error\n");
-		goto err_release_group_1;
+	if (is_s10) {
+		/* Use IRQ to determine SError origin instead of assigning IRQ */
+		rc = of_property_read_u32_index(np, "interrupts", 1, &altdev->db_irq);
+		if (rc) {
+			edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
+			goto err_release_group_1;
+		}
+	} else {
+		altdev->db_irq = irq_of_parse_and_map(np, 3);
+		if (!altdev->db_irq) {
+			edac_printk(KERN_ERR, EDAC_DEVICE, "Error PortB DBIRQ alloc\n");
+			rc = -ENODEV;
+			goto err_release_group_1;
+		}
+		rc = devm_request_irq(&altdev->ddev, altdev->db_irq,
+				      prv->ecc_irq_handler, IRQF_TRIGGER_HIGH,
+				      ecc_name, altdev);
+		if (rc) {
+			edac_printk(KERN_ERR, EDAC_DEVICE, "PortB DBERR IRQ error\n");
+			goto err_release_group_1;
+		}
 	}
-#endif
 
 	rc = edac_device_add_device(dci);
 	if (rc) {
@@ -1974,29 +1973,29 @@ static int altr_edac_a10_device_add(struct altr_arria10_edac *edac,
 		goto err_release_group1;
 	}
 
-#ifdef CONFIG_64BIT
-	/* Use IRQ to determine SError origin instead of assigning IRQ */
-	rc = of_property_read_u32_index(np, "interrupts", 0, &altdev->db_irq);
-	if (rc) {
-		edac_printk(KERN_ERR, EDAC_DEVICE,
-			    "Unable to parse DB IRQ index\n");
-		goto err_release_group1;
-	}
-#else
-	altdev->db_irq = irq_of_parse_and_map(np, 1);
-	if (!altdev->db_irq) {
-		edac_printk(KERN_ERR, EDAC_DEVICE, "Error allocating DBIRQ\n");
-		rc = -ENODEV;
-		goto err_release_group1;
-	}
-	rc = devm_request_irq(edac->dev, altdev->db_irq, prv->ecc_irq_handler,
-			      IRQF_TRIGGER_HIGH,
-			      ecc_name, altdev);
-	if (rc) {
-		edac_printk(KERN_ERR, EDAC_DEVICE, "No DBERR IRQ resource\n");
-		goto err_release_group1;
+	if (edac->is_s10) {
+		/* Use IRQ to determine SError origin instead of assigning IRQ */
+		rc = of_property_read_u32_index(np, "interrupts", 0, &altdev->db_irq);
+		if (rc) {
+			edac_printk(KERN_ERR, EDAC_DEVICE,
+				    "Unable to parse DB IRQ index\n");
+			goto err_release_group1;
+		}
+	} else {
+		altdev->db_irq = irq_of_parse_and_map(np, 1);
+		if (!altdev->db_irq) {
+			edac_printk(KERN_ERR, EDAC_DEVICE, "Error allocating DBIRQ\n");
+			rc = -ENODEV;
+			goto err_release_group1;
+		}
+		rc = devm_re
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`chained_irq_enter`
- 外部函数：`chained_irq_exit`
- 外部函数：`debugfs_remove_recursive`
- 外部函数：`devm_ioremap_resource`
- 外部函数：`devm_request_irq`
- 外部函数：`devres_open_group`
- 外部函数：`edac_debugfs_create_dir`
- 外部函数：`edac_debugfs_create_file`
- 外部函数：`edac_device_add_device`
- 外部函数：`edac_device_alloc_ctl_info`
- 外部函数：`edac_device_alloc_index`
- 外部函数：`edac_printk`
- 外部函数：`for_each_set_bit`
- 外部函数：`generic_handle_domain_irq`
- 外部函数：`irq_desc_get_chip`
- 外部函数：`irq_desc_get_handler_data`
- 外部函数：`irq_desc_get_irq`
- 外部函数：`irq_of_parse_and_map`
- 外部函数：`of_address_to_resource`
- 外部函数：`of_device_is_available`
- 外部函数：`of_device_is_compatible`
- 外部函数：`of_match_node`
- 外部函数：`of_node_put`
- 外部函数：`of_parse_phandle`
- 外部函数：`of_property_read_u32_index`
- 外部函数：`readl`
- 外部函数：`regmap_read`
- 外部函数：`setup`
- 大写宏：`A10`
- 大写宏：`A10_SYSMGR_ECC_INTSTAT_DERR_OFST`
- 大写宏：`A10_SYSMGR_ECC_INTSTAT_SERR_OFST`
- 大写宏：`CONFIG_64BIT`
- 大写宏：`CONFIG_EDAC_DEBUG`
- 大写宏：`DBERR`
- 大写宏：`DBIRQ`
- 大写宏：`ECC`
- 大写宏：`EDAC`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`FIXME`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IRQ`
- 大写宏：`IRQF_TRIGGER_HIGH`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`KERN_ERR`
- 大写宏：`PTR_ERR`
- 大写宏：`S10`
- 大写宏：`SBIRQ`
- 大写宏：`SDRAM`
- 大写宏：`S_IWUSR`
- 外部类型：`Altera`
- 外部类型：`Check`
- 外部类型：`Ensure`
- 外部类型：`Error`
- 外部类型：`Get`
- 外部类型：`IRQs`
- 外部类型：`Index`
- 外部类型：`Instead`
- 外部类型：`Manager`
- 外部类型：`No`
- 外部类型：`PortB`
- 外部类型：`SError`
- 外部类型：`Unable`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`altr_arria10_edac`
- 外部类型：`altr_edac_device_dev`
- 外部类型：`device_node`
- 外部类型：`edac_device_ctl_info`
- 外部类型：`edac_device_prv_data`
- 外部类型：`irq_chip`
- 外部类型：`irq_desc`
- 外部类型：`of_device_id`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-6a9f80fb4c` → 本草稿移入 `cases/defect/auto-linux-6a9f80fb4c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
