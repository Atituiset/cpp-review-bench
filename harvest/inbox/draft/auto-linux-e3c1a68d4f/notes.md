# auto-linux-e3c1a68d4f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#fddfc3ec31799a932bb92f1b8a84cb3d1f963be9](https://github.com/torvalds/linux/commit/fddfc3ec31799a932bb92f1b8a84cb3d1f963be9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 161 |
| 编译错误数（gcc syntax-only） | 227（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fddfc3ec31799a932bb92f1b8a84cb3d1f963be9 (https://github.com/torvalds/linux/commit/fddfc3ec31799a932bb92f1b8a84cb3d1f963be9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2418；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fddfc3ec31799a932bb92f1b8a84cb3d1f963be9 Merge tag 'pci-v7.3-fixes-2' of git://git.kernel.org/pub/scm/linux/kernel/git/pci/pci :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2380,6 +2380,7 @@ int pci_do_resource_release_and_resize(struct pci_dev *pdev, int resno, int size
 	struct resource *res = pci_resource_n(pdev, resno);
 	struct pci_dev_resource *dev_res;
 	struct pci_bus *bus = pdev->bus;
+	struct pci_dev *bridge = pci_upstream_bridge(pdev);
 	struct resource *b_win, *r;
 	LIST_HEAD(saved);
 	unsigned int i;
@@ -2397,6 +2398,8 @@ int pci_do_resource_release_and_resize(struct pci_dev *pdev, int resno, int size
 	if (ret)
 		return ret;
 
+	down_read(&pci_bus_sem);
+
 	pci_dev_for_each_resource(pdev, r, i) {
 		if (i >= PCI_BRIDGE_RESOURCES)
 			break;
@@ -2415,13 +2418,21 @@ int pci_do_resource_release_and_resize(struct pci_dev *pdev, int resno, int size
 
 	pci_resize_resource_set_size(pdev, resno, size);
 
-	if (!bus->self)
-		goto out;
+	if (bridge) {
+		ret = pbus_reassign_bridge_resources(bus, res, &saved);
+		if (ret)
+			goto restore;
+	} else {
+		/* No bridge window to adjust; let the core reassign the bus. */
+		pci_bus_assign_resources(bus);
 
-	down_read(&pci_bus_sem);
-	ret = pbus_reassign_bridge_resources(bus, res, &saved);
-	if (ret)
-		goto restore;
+		list_for_each_entry(dev_res, &saved, list) {
+			if (!resource_assigned(dev_res->res)) {
+				ret = -ENOSPC;
+				goto restore;
+			}
+		}
+	}
 
 out:
 	up_read(&pci_bus_sem);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ffs`
- 外部函数：`assign_fixed_resource_on_bus`
- 外部函数：`defined`
- 外部函数：`down_read`
- 外部函数：`failed`
- 外部函数：`kfree`
- 外部函数：`kzalloc`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`list_move_tail`
- 外部函数：`max`
- 外部函数：`max_t`
- 外部函数：`pR`
- 外部函数：`panic`
- 外部函数：`pci_assign_resource`
- 外部函数：`pci_bus_resource_n`
- 外部函数：`pci_bus_size_cardbus_bridge`
- 外部函数：`pci_dbg`
- 外部函数：`pci_domain_nr`
- 外部函数：`pci_info`
- 外部函数：`pci_is_enabled`
- 外部函数：`pci_is_root_bus`
- 外部函数：`pci_read_config_word`
- 外部函数：`pci_reassign_resource`
- 外部函数：`pci_release_resource`
- 外部函数：`pci_resize_resource_set_size`
- 外部函数：`pci_resource_alignment`
- 外部函数：`pci_resource_is_bridge_win`
- 外部函数：`pci_resource_is_iov`
- 外部函数：`pci_resource_n`
- 外部函数：`pci_resource_name`
- 外部函数：`pci_resource_num`
- 外部函数：`pci_setup_cardbus_bridge`
- 外部函数：`pci_warn`
- 外部函数：`pci_write_config_dword`
- 外部函数：`pci_write_config_word`
- 外部函数：`pcibios_resource_to_bus`
- 外部函数：`released`
- 外部函数：`resource_assigned`
- 外部函数：`resource_set_range`
- 外部函数：`resource_size`
- 外部函数：`to_pci_host_bridge`
- 外部函数：`up_read`
- 外部函数：`upper_32_bits`
- 外部函数：`yet`
- 大写宏：`ALIGN`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`CONFIG_EISA`
- 大写宏：`CONFIG_ISA`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSPC`
- 大写宏：`HAVE_ISA`
- 大写宏：`IOAPIC`
- 大写宏：`IORESOURCE_DISABLED`
- 大写宏：`IORESOURCE_IO`
- 大写宏：`IORESOURCE_MEM`
- 大写宏：`IORESOURCE_MEM_64`
- 大写宏：`IORESOURCE_PCI_FIXED`
- 大写宏：`IORESOURCE_PREFETCH`
- 大写宏：`IORESOURCE_ROM_ENABLE`
- 大写宏：`IORESOURCE_SIZEALIGN`
- 大写宏：`IORESOURCE_STARTALIGN`
- 大写宏：`IORESOURCE_TYPE_BITS`
- 大写宏：`IORESOURCE_UNSET`
- 大写宏：`IOV`
- 大写宏：`ISA`
- 大写宏：`IS_ALIGNED`
- 大写宏：`LIST_HEAD`
- 大写宏：`MMIO`
- 大写宏：`NULL`
- 大写宏：`PCI`
- 大写宏：`PCI_BRIDGE_CONTROL`
- 大写宏：`PCI_BRIDGE_IO_WINDOW`
- 大写宏：`PCI_BRIDGE_MEM_WINDOW`
- 大写宏：`PCI_BRIDGE_PREF_MEM_WINDOW`
- 大写宏：`PCI_BRIDGE_RESOURCES`
- 大写宏：`PCI_BUS_BRIDGE_IO_WINDOW`
- 大写宏：`PCI_BUS_BRIDGE_MEM_WINDOW`
- 大写宏：`PCI_BUS_BRIDGE_PREF_MEM_WINDOW`
- 大写宏：`PCI_CLASS_BRIDGE_CARDBUS`
- 大写宏：`PCI_CLASS_BRIDGE_HOST`
- 大写宏：`PCI_CLASS_BRIDGE_PCI`
- 大写宏：`PCI_CLASS_NOT_DEFINED`
- 大写宏：`PCI_CLASS_SYSTEM_PIC`
- 大写宏：`PCI_COMMAND`
- 大写宏：`PCI_COMMAND_IO`
- 大写宏：`PCI_COMMAND_MEMORY`
- 大写宏：`PCI_HEADER_TYPE_BRIDGE`
- 大写宏：`PCI_HEADER_TYPE_CARDBUS`
- 大写宏：`PCI_IO_1K_RANGE_MASK`
- 大写宏：`PCI_IO_BASE`
- 大写宏：`PCI_IO_BASE_UPPER16`
- 大写宏：`PCI_IO_RANGE_MASK`
- 大写宏：`PCI_MEMORY_BASE`
- 大写宏：`PCI_PREF_BASE_UPPER32`
- 大写宏：`PCI_PREF_LIMIT_UPPER32`
- 大写宏：`PCI_PREF_MEMORY_BASE`
- 大写宏：`PCI_PREF_RANGE_TYPE_64`
- 大写宏：`PCI_ROM_RESOURCE`
- 大写宏：`PREF`
- 大写宏：`ROM`
- 大写宏：`SZ_1K`
- 大写宏：`SZ_1M`
- 大写宏：`SZ_4K`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Account`
- 外部类型：`After`
- 外部类型：`Alignments`
- 外部类型：`All`
- 外部类型：`As`
- 外部类型：`BARs`
- 外部类型：`Bridge`
- 外部类型：`CardBuses`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Count`
- 外部类型：`Don`
- 外部类型：`Fallback`
- 外部类型：`Here`
- 外部类型：`IOAPICs`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`Insert`
- 外部类型：`Intentionally`
- 外部类型：`Memory`
- 外部类型：`Might`
- 外部类型：`One`
- 外部类型：`Only`
- 外部类型：`Per`
- 外部类型：`Put`
- 外部类型：`Release`
- 外部类型：`Remove`
- 外部类型：`Reset`
- 外部类型：`Restore`
- 外部类型：`Satisfy`
- 外部类型：`Save`
- 外部类型：`Separate`
- 外部类型：`Set`
- 外部类型：`Should`
- 外部类型：`Skip`
- 外部类型：`Take`
- 外部类型：`Temporarily`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`They`
- 外部类型：`To`
- 外部类型：`Try`
- 外部类型：`Update`
- 外部类型：`Will`
- 外部类型：`Without`
- 外部类型：`list_head`
- 外部类型：`pci_bus`
- 外部类型：`pci_bus_region`
- 外部类型：`pci_dev`
- 外部类型：`pci_host_bridge`
- 外部类型：`resource`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-e3c1a68d4f` → 本草稿移入 `cases/defect/auto-linux-e3c1a68d4f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
