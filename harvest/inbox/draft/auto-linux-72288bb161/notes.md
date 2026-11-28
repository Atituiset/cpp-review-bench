# auto-linux-72288bb161

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c639ac6b9cf100b860408a3cf8d7eab268866cd7](https://github.com/torvalds/linux/commit/c639ac6b9cf100b860408a3cf8d7eab268866cd7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 51 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c639ac6b9cf100b860408a3cf8d7eab268866cd7 (https://github.com/torvalds/linux/commit/c639ac6b9cf100b860408a3cf8d7eab268866cd7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 459；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c639ac6b9cf100b860408a3cf8d7eab268866cd7 Merge tag 'drm-fixes-2026-11-28' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -369,7 +369,7 @@ static bool amdgpu_atrm_get_bios(struct amdgpu_device *adev)
 	bool found = false;
 
 	/* ATRM is for on-platform devices only */
-	if (dev_is_removable(&adev->pdev->dev))
+	if (amdgpu_device_is_removable(adev))
 		return false;
 
 	while ((pdev = pci_get_base_class(PCI_BASE_CLASS_DISPLAY, pdev))) {
@@ -436,35 +436,23 @@ static bool amdgpu_read_disabled_bios(struct amdgpu_device *adev)
  * VFCT entries contain the PCI bus number as recorded during BIOS POST.
  * On systems where the kernel renumbers PCI buses (e.g. pci=realloc or
  * resource conflicts), the runtime bus number may differ from the POST
- * value.  Match by device identity (vendor + device + function) and use
- * the bus number as a preference: exact bus match is preferred, but when
- * the bus numbers disagree we accept the entry if the device identity
- * matches.
+ * value.  Match against the secondary bus number of the upstream bridge
+ * at FW/OS handoff, which the PCI core caches before any renumbering.
  *
- * Returns: 0 on match, -ENODEV on no match
+ * Returns: true if @vhdr describes this device, false otherwise
  */
-static int amdgpu_acpi_vfct_match(struct amdgpu_device *adev,
-				  VFCT_IMAGE_HEADER *vhdr)
+static bool amdgpu_acpi_vfct_match(struct amdgpu_device *adev,
+				   VFCT_IMAGE_HEADER *vhdr)
 {
-	/* Vendor and device IDs must always match */
-	if (vhdr->VendorID != adev->pdev->vendor ||
-	    vhdr->DeviceID != adev->pdev->device)
-		return -ENODEV;
-
-	if (vhdr->PCIDevice != PCI_SLOT(adev->pdev->devfn) ||
-	    vhdr->PCIFunction != PCI_FUNC(adev->pdev->devfn))
-		return -ENODEV;
-
-	/* Exact bus number match - preferred */
-	if (vhdr->PCIBus == adev->pdev->bus->number)
-		return 0;
-
-	/* Bus mismatch but device identity matches (PCI renumbering case) */
-	dev_notice(adev->dev,
-		   "VFCT bus number mismatch: table %u != runtime %u, matching by device identity (vendor 0x%04x device 0x%04x)\n",
-		   vhdr->PCIBus, adev->pdev->bus->number,
-		   adev->pdev->vendor, adev->pdev->device);
-	return 0;
+	struct pci_dev *pdev = adev->pdev;
+	struct pci_dev *bridge = pci_upstream_bridge(pdev);
+	u8 bus = bridge ? bridge->fw_sec_busnr : pdev->bus->number;
+
+	return vhdr->VendorID == pdev->vendor &&
+	       vhdr->DeviceID == pdev->device &&
+	       vhdr->PCIBus == bus &&
+	       vhdr->PCIDevice == PCI_SLOT(pdev->devfn) &&
+	       vhdr->PCIFunction == PCI_FUNC(pdev->devfn);
 }
 
 static bool amdgpu_acpi_vfct_bios(struct amdgpu_device *adev)
@@ -503,7 +491,7 @@ static bool amdgpu_acpi_vfct_bios(struct amdgpu_device *adev)
 		}
 
 		if (vhdr->ImageLength &&
-		    !amdgpu_acpi_vfct_match(adev, vhdr)) {
+		    amdgpu_acpi_vfct_match(adev, vhdr)) {
 			adev->bios = kmemdup(&vbios->VbiosContent,
 					     vhdr->ImageLength,
 					     GFP_KERNEL);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acpi_get_table`
- 外部函数：`acpi_put_table`
- 外部函数：`amdgpu_acpi_vfct_match`
- 外部函数：`broken`
- 外部函数：`dev_dbg`
- 外部函数：`dev_info`
- 外部函数：`dev_is_removable`
- 外部函数：`dev_notice`
- 外部函数：`identity`
- 外部函数：`kfree`
- 外部函数：`kmemdup`
- 外部函数：`matches`
- 外部函数：`memcmp`
- 外部函数：`pci_get_base_class`
- 大写宏：`ACPI`
- 大写宏：`ACPI_SUCCESS`
- 大写宏：`ATOM`
- 大写宏：`ATOMBIOS`
- 大写宏：`ATRM`
- 大写宏：`BIOS`
- 大写宏：`ENODEV`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GOP_VBIOS_CONTENT`
- 大写宏：`MOTA`
- 大写宏：`NULL`
- 大写宏：`PCI`
- 大写宏：`PCI_BASE_CLASS_DISPLAY`
- 大写宏：`PCI_FUNC`
- 大写宏：`PCI_SLOT`
- 大写宏：`POST`
- 大写宏：`UEFI_ACPI_VFCT`
- 大写宏：`VBIOS`
- 大写宏：`VFCT`
- 大写宏：`VFCT_IMAGE_HEADER`
- 外部类型：`Bus`
- 外部类型：`Can`
- 外部类型：`DeviceID`
- 外部类型：`Exact`
- 外部类型：`IDs`
- 外部类型：`ImageLength`
- 外部类型：`Match`
- 外部类型：`On`
- 外部类型：`PCIBus`
- 外部类型：`PCIDevice`
- 外部类型：`PCIFunction`
- 外部类型：`Returns`
- 外部类型：`VBIOSImageOffset`
- 外部类型：`VbiosContent`
- 外部类型：`VbiosHeader`
- 外部类型：`Vendor`
- 外部类型：`VendorID`
- 外部类型：`acpi_table_header`
- 外部类型：`amdgpu_device`
- 外部类型：`size_t`
- 外部类型：`uint16_t`
- 外部类型：`uint8_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-72288bb161` → 本草稿移入 `cases/defect/auto-linux-72288bb161/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
