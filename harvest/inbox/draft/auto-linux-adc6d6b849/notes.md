# auto-linux-adc6d6b849

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4766082b791b515953d87f7a24c2ea857e6c10b0](https://github.com/torvalds/linux/commit/4766082b791b515953d87f7a24c2ea857e6c10b0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4766082b791b515953d87f7a24c2ea857e6c10b0 (https://github.com/torvalds/linux/commit/4766082b791b515953d87f7a24c2ea857e6c10b0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 1271；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4766082b791b515953d87f7a24c2ea857e6c10b0 Merge tag 'pci-v7.3-fixes-6' of git://git.kernel.org/pub/scm/linux/kernel/git/pci/pci :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1246,9 +1246,11 @@ static int pci_dev_wait(struct pci_dev *dev, char *reset_type, int timeout)
 	 * initialize.
 	 *
 	 * If the device is below a Root Port with Configuration RRS
-	 * Software Visibility enabled, reading the Vendor ID returns a
+	 * Software Visibility enabled, reading the Vendor ID may return a
 	 * special data value if the device responded with RRS.  Read the
-	 * Vendor ID until we get non-RRS status.
+	 * Vendor ID until we get non-RRS status and something other than
+	 * ~0.  The Vendor ID of VFs reads ~0, so check their Command
+	 * register instead.
 	 *
 	 * If there's no Root Port or Configuration RRS Software Visibility
 	 * is not enabled, the device may still respond with RRS, but
@@ -1268,8 +1270,13 @@ static int pci_dev_wait(struct pci_dev *dev, char *reset_type, int timeout)
 
 		if (root && root->config_rrs_sv) {
 			pci_read_config_dword(dev, PCI_VENDOR_ID, &id);
-			if (!pci_bus_rrs_vendor_id(id))
-				break;
+			if (!pci_bus_rrs_vendor_id(id)) {
+				if (PCI_POSSIBLE_ERROR(id) && dev->is_virtfn)
+					pci_read_config_dword(dev, PCI_COMMAND,
+							      &id);
+				if (!PCI_POSSIBLE_ERROR(id))
+					break;
+			}
 		} else {
 			pci_read_config_dword(dev, PCI_COMMAND, &id);
 			if (!PCI_POSSIBLE_ERROR(id))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pci_bus_rrs_vendor_id`
- 外部函数：`pci_read_config_dword`
- 大写宏：`PCI_COMMAND`
- 大写宏：`PCI_POSSIBLE_ERROR`
- 大写宏：`PCI_VENDOR_ID`
- 大写宏：`RRS`
- 外部类型：`Configuration`
- 外部类型：`If`
- 外部类型：`Port`
- 外部类型：`Read`
- 外部类型：`Root`
- 外部类型：`Software`
- 外部类型：`Vendor`
- 外部类型：`Visibility`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-adc6d6b849` → 本草稿移入 `cases/defect/auto-linux-adc6d6b849/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
