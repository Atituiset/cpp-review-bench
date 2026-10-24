# auto-linux-28e10d2b5d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#17bb0e2f6c8043f70a693a5f3797471cbed354c8](https://github.com/torvalds/linux/commit/17bb0e2f6c8043f70a693a5f3797471cbed354c8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 68 |
| 编译错误数（gcc syntax-only） | 65（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #17bb0e2f6c8043f70a693a5f3797471cbed354c8 (https://github.com/torvalds/linux/commit/17bb0e2f6c8043f70a693a5f3797471cbed354c8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1031；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 17bb0e2f6c8043f70a693a5f3797471cbed354c8 Merge tag 'pci-v7.4-changes' of git://git.kernel.org/pub/scm/linux/kernel/git/pci/pci :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -993,6 +993,25 @@ static void acpi_pci_root_release_info(struct pci_host_bridge *bridge)
 	__acpi_pci_root_release_info(bridge->release_data);
 }
 
+#define OSC_OWNER(ctrl, bit, flag)		\
+	do {					\
+		if (!((ctrl) & (bit)))		\
+			flag = 0;		\
+	} while (0)
+
+#define FLAG(x)		((x) ? '+' : '-')
+
+/*
+ * _OSC control bits for the features implemented by the PCIe port driver,
+ * i.e., the ones "pcie_ports=native" applies to.  LTR and SHPC hotplug are
+ * negotiated via _OSC as well, but they are not portdrv services, so
+ * "pcie_ports=" has no bearing on them.
+ */
+#define OSC_PCIE_PORT_SERVICE_CONTROLS	(OSC_PCI_EXPRESS_NATIVE_HP_CONTROL | \
+					 OSC_PCI_EXPRESS_PME_CONTROL | \
+					 OSC_PCI_EXPRESS_AER_CONTROL | \
+					 OSC_PCI_EXPRESS_DPC_CONTROL)
+
 struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
 				     struct acpi_pci_root_ops *ops,
 				     struct acpi_pci_root_info *info,
@@ -1003,6 +1022,7 @@ struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
 	int node = acpi_get_node(device->handle);
 	struct pci_bus *bus;
 	struct pci_host_bridge *host_bridge;
+	u32 ctrl, ext_ctrl, override = 0;
 
 	info->root = root;
 	info->bridge = device;
@@ -1028,21 +1048,49 @@ struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
 		goto out_release_info;
 
 	host_bridge = to_pci_host_bridge(bus->bridge);
-	if (!(root->osc_control_set & OSC_PCI_EXPRESS_NATIVE_HP_CONTROL))
-		host_bridge->native_pcie_hotplug = 0;
-	if (!(root->osc_control_set & OSC_PCI_SHPC_NATIVE_HP_CONTROL))
-		host_bridge->native_shpc_hotplug = 0;
-	if (!(root->osc_control_set & OSC_PCI_EXPRESS_AER_CONTROL))
-		host_bridge->native_aer = 0;
-	if (!(root->osc_control_set & OSC_PCI_EXPRESS_PME_CONTROL))
-		host_bridge->native_pme = 0;
-	if (!(root->osc_control_set & OSC_PCI_EXPRESS_LTR_CONTROL))
-		host_bridge->native_ltr = 0;
-	if (!(root->osc_control_set & OSC_PCI_EXPRESS_DPC_CONTROL))
-		host_bridge->native_dpc = 0;
-
-	if (!(root->osc_ext_control_set & OSC_CXL_ERROR_REPORTING_CONTROL))
-		host_bridge->native_cxl_error = 0;
+
+	ctrl = root->osc_control_set;
+	ext_ctrl = root->osc_ext_control_set;
+
+	/*
+	 * If the user specified "pcie_ports=native", use the PCIe port
+	 * services regardless of what _OSC says, i.e., proceed as though the
+	 * platform had granted us control of them.  "pcie_ports=dpc-native"
+	 * does the same for DPC alone.  This may conflict with firmware that
+	 * expects to own those features.
+	 */
+	if (pcie_ports_native)
+		override = OSC_PCIE_PORT_SERVICE_CONTROLS & ~ctrl;
+	else if (pcie_ports_dpc_native)
+		override = OSC_PCI_EXPRESS_DPC_CONTROL & ~ctrl;
+
+	if (override) {
+		decode_osc_control(root, pcie_ports_native ?
+				   "OS forcing control (\"pcie_ports=native\") of" :
+				   "OS forcing control (\"pcie_ports=dpc-native\") of",
+				   override);
+		ctrl |= override;
+	}
+
+	OSC_OWNER(ctrl, OSC_PCI_EXPRESS_NATIVE_HP_CONTROL,
+		  host_bridge->native_pcie_hotplug);
+	OSC_OWNER(ctrl, OSC_PCI_SHPC_NATIVE_HP_CONTROL,
+		  host_bridge->native_shpc_hotplug);
+	OSC_OWNER(ctrl, OSC_PCI_EXPRESS_AER_CONTROL, host_bridge->native_aer);
+	OSC_OWNER(ctrl, OSC_PCI_EXPRESS_PME_CONTROL, host_bridge->native_pme);
+	OSC_OWNER(ctrl, OSC_PCI_EXPRESS_LTR_CONTROL, host_bridge->native_ltr);
+	OSC_OWNER(ctrl, OSC_PCI_EXPRESS_DPC_CONTROL, host_bridge->native_dpc);
+
+	OSC_OWNER(ext_ctrl, OSC_CXL_ERROR_REPORTING_CONTROL,
+		  host_bridge->native_cxl_error);
+
+	dev_info(&root->device->dev, "OS native features: SHPCHotplug%c PCIeHotplug%c PME%c AER%c DPC%c LTR%c\n",
+		 FLAG(host_bridge->native_shpc_hotplug),
+		 FLAG(host_bridge->native_pcie_hotplug),
+		 FLAG(host_bridge->native_pme),
+		 FLAG(host_bridge->native_aer),
+		 FLAG(host_bridge->native_dpc),
+		 FLAG(host_bridge->native_ltr));
 
 	acpi_dev_power_up_children_with_adr(device);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acpi_dev_get_resources`
- 外部函数：`acpi_dev_power_up_children_with_adr`
- 外部函数：`acpi_get_node`
- 外部函数：`dev_dbg`
- 外部函数：`dev_info`
- 外部函数：`dev_printk`
- 外部函数：`dev_warn`
- 外部函数：`init_info`
- 外部函数：`insert_resource_conflict`
- 外部函数：`list_splice_init`
- 外部函数：`min`
- 外部函数：`pR`
- 外部函数：`pci_add_resource`
- 外部函数：`pci_address_to_pio`
- 外部函数：`pci_create_root_bus`
- 外部函数：`pci_register_io_range`
- 外部函数：`pci_remap_iospace`
- 外部函数：`pci_scan_child_bus`
- 外部函数：`pci_set_host_bridge_release`
- 外部函数：`pci_unmap_iospace`
- 外部函数：`pr_info`
- 外部函数：`prepare_resources`
- 外部函数：`release_info`
- 外部函数：`release_resource`
- 外部函数：`resource_list_add_tail`
- 外部函数：`resource_list_del`
- 外部函数：`resource_list_destroy_entry`
- 外部函数：`resource_list_free_entry`
- 外部函数：`resource_size`
- 外部函数：`resource_union`
- 外部函数：`snprintf`
- 外部函数：`to_pci_host_bridge`
- 大写宏：`ACPI`
- 大写宏：`BUG_ON`
- 大写宏：`CPU`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`IORESOURCE_DISABLED`
- 大写宏：`IORESOURCE_IO`
- 大写宏：`IORESOURCE_MEM`
- 大写宏：`IORESOURCE_MEM_8AND16BIT`
- 大写宏：`KERN_DEBUG`
- 大写宏：`LIST_HEAD`
- 大写宏：`NULL`
- 大写宏：`NUMA`
- 大写宏：`NUMA_NO_NODE`
- 大写宏：`OSC_CXL_ERROR_REPORTING_CONTROL`
- 大写宏：`OSC_PCI_EXPRESS_AER_CONTROL`
- 大写宏：`OSC_PCI_EXPRESS_DPC_CONTROL`
- 大写宏：`OSC_PCI_EXPRESS_LTR_CONTROL`
- 大写宏：`OSC_PCI_EXPRESS_NATIVE_HP_CONTROL`
- 大写宏：`OSC_PCI_EXPRESS_PME_CONTROL`
- 大写宏：`OSC_PCI_SHPC_NATIVE_HP_CONTROL`
- 大写宏：`PCI`
- 大写宏：`PCI_IOBASE`
- 外部类型：`Bus`
- 外部类型：`Exclude`
- 外部类型：`Remapped`
- 外部类型：`Some`
- 外部类型：`acpi_device`
- 外部类型：`acpi_pci_root`
- 外部类型：`acpi_pci_root_info`
- 外部类型：`acpi_pci_root_ops`
- 外部类型：`device`
- 外部类型：`fwnode_handle`
- 外部类型：`list_head`
- 外部类型：`pci_bus`
- 外部类型：`pci_host_bridge`
- 外部类型：`resource`
- 外部类型：`resource_entry`
- 外部类型：`resource_size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-28e10d2b5d` → 本草稿移入 `cases/defect/auto-linux-28e10d2b5d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
