# auto-linux-e437aa70d9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0](https://github.com/torvalds/linux/commit/3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 144 |
| 编译错误数（gcc syntax-only） | 111（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0 (https://github.com/torvalds/linux/commit/3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1388；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0 Merge tag 'pci-v7.3-fixes-4' of git://git.kernel.org/pub/scm/linux/kernel/git/pci/pci :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1360,8 +1360,10 @@ static DEFINE_KFIFO(aer_recover_ring, struct aer_recover_entry,
 
 static void aer_recover_work_func(struct work_struct *work)
 {
+	struct aer_capability_regs *regs;
 	struct aer_recover_entry entry;
 	struct pci_dev *pdev;
+	u32 err;
 
 	while (kfifo_get(&aer_recover_ring, &entry)) {
 		pdev = pci_get_domain_bus_and_slot(entry.domain, entry.bus,
@@ -1375,6 +1377,12 @@ static void aer_recover_work_func(struct work_struct *work)
 		}
 		pci_print_aer(pdev, entry.severity, entry.regs);
 
+		regs = entry.regs;
+		if (entry.severity == AER_CORRECTABLE)
+			err = regs->cor_status & ~regs->cor_mask;
+		else
+			err = regs->uncor_status & ~regs->uncor_mask;
+
 		/*
 		 * Memory for aer_capability_regs(entry.regs) is being
 		 * allocated from the ghes_estatus_pool to protect it from
@@ -1385,12 +1393,14 @@ static void aer_recover_work_func(struct work_struct *work)
 		ghes_estatus_pool_region_free((unsigned long)entry.regs,
 					    sizeof(struct aer_capability_regs));
 
-		if (entry.severity == AER_NONFATAL)
-			pcie_do_recovery(pdev, pci_channel_io_normal,
-					 aer_root_reset);
-		else if (entry.severity == AER_FATAL)
-			pcie_do_recovery(pdev, pci_channel_io_frozen,
-					 aer_root_reset);
+		if (err) {
+			if (entry.severity == AER_NONFATAL)
+				pcie_do_recovery(pdev, pci_channel_io_normal,
+						 aer_root_reset);
+			else if (entry.severity == AER_FATAL)
+				pcie_do_recovery(pdev, pci_channel_io_frozen,
+						 aer_root_reset);
+		}
 		pci_dev_put(pdev);
 	}
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ratelimit`
- 外部函数：`aer_capability_regs`
- 外部函数：`aer_err_bus`
- 外部函数：`aer_printk`
- 外部函数：`dev_fmt`
- 外部函数：`dev_printk`
- 外部函数：`ghes_estatus_pool_region_free`
- 外部函数：`hwerr_log_error_type`
- 外部函数：`internally`
- 外部函数：`itself`
- 外部函数：`kfifo_get`
- 外部函数：`logged`
- 外部函数：`partner`
- 外部函数：`pci_bus_error_reset`
- 外部函数：`pci_dev_put`
- 外部函数：`pci_find_host_bridge`
- 外部函数：`pci_get_domain_bus_and_slot`
- 外部函数：`pci_info`
- 外部函数：`pci_is_root_bus`
- 外部函数：`pci_name`
- 外部函数：`pci_pcie_type`
- 外部函数：`pci_read_config_dword`
- 外部函数：`pci_write_config_dword`
- 外部函数：`pcie_do_recovery`
- 外部函数：`pcie_find_root_port`
- 外部函数：`pcie_is_cxl`
- 外部函数：`pcie_print_tlp_log`
- 外部函数：`pcie_reset_flr`
- 外部函数：`pr_fmt`
- 外部函数：`rcec`
- 外部函数：`reset`
- 外部函数：`trace_aer_event`
- 大写宏：`AER`
- 大写宏：`AER_CORRECTABLE`
- 大写宏：`AER_FATAL`
- 大写宏：`AER_NONFATAL`
- 大写宏：`BIT`
- 大写宏：`ERR_COR`
- 大写宏：`ERR_FATAL`
- 大写宏：`ERR_NONFATAL`
- 大写宏：`FLR`
- 大写宏：`HWERR_RECOV_PCI`
- 大写宏：`INCLUDING`
- 大写宏：`KERN_ERR`
- 大写宏：`KERN_WARNING`
- 大写宏：`NULL`
- 大写宏：`PCI_ERR_CAP_COMP_TIME_LOG`
- 大写宏：`PCI_ERR_CAP_FEP`
- 大写宏：`PCI_ERR_COR_ADV_NFAT`
- 大写宏：`PCI_ERR_COR_BAD_DLLP`
- 大写宏：`PCI_ERR_COR_BAD_TLP`
- 大写宏：`PCI_ERR_COR_INTERNAL`
- 大写宏：`PCI_ERR_COR_LOG_OVER`
- 大写宏：`PCI_ERR_COR_RCVR`
- 大写宏：`PCI_ERR_COR_REP_ROLL`
- 大写宏：`PCI_ERR_COR_REP_TIMER`
- 大写宏：`PCI_ERR_COR_STATUS`
- 大写宏：`PCI_ERR_ROOT_CMD_COR_EN`
- 大写宏：`PCI_ERR_ROOT_CMD_FATAL_EN`
- 大写宏：`PCI_ERR_ROOT_CMD_NONFATAL_EN`
- 大写宏：`PCI_ERR_ROOT_COMMAND`
- 大写宏：`PCI_ERR_ROOT_STATUS`
- 大写宏：`PCI_ERR_UNCOR_STATUS`
- 大写宏：`PCI_ERR_UNC_ACSV`
- 大写宏：`PCI_ERR_UNC_ATOMEG`
- 大写宏：`PCI_ERR_UNC_COMP_ABORT`
- 大写宏：`PCI_ERR_UNC_COMP_TIME`
- 大写宏：`PCI_ERR_UNC_DLP`
- 大写宏：`PCI_ERR_UNC_DMWR_BLK`
- 大写宏：`PCI_ERR_UNC_ECRC`
- 大写宏：`PCI_ERR_UNC_IDE_CHECK`
- 大写宏：`PCI_ERR_UNC_INTN`
- 大写宏：`PCI_ERR_UNC_MALF_TLP`
- 大写宏：`PCI_ERR_UNC_MCBTLP`
- 大写宏：`PCI_ERR_UNC_MISR_IDE`
- 大写宏：`PCI_ERR_UNC_PCRC_CHECK`
- 大写宏：`PCI_ERR_UNC_POISON_BLK`
- 大写宏：`PCI_ERR_UNC_POISON_TLP`
- 大写宏：`PCI_ERR_UNC_SURPDN`
- 大写宏：`PCI_ERR_UNC_TLPPRE`
- 大写宏：`PCI_ERR_UNC_UNSUP`
- 大写宏：`PCI_ERR_UNC_UNX_COMP`
- 大写宏：`PCI_ERR_UNC_XLAT_BLK`
- 大写宏：`PCI_ERS_RESULT_DISCONNECT`
- 大写宏：`PCI_ERS_RESULT_RECOVERED`
- 大写宏：`PCI_EXP_DEVSTA_CED`
- 大写宏：`PCI_EXP_DEVSTA_NFED`
- 大写宏：`PCI_EXP_TYPE_RC_EC`
- 大写宏：`PCI_EXP_TYPE_RC_END`
- 大写宏：`PCI_RESET_DO_RESET`
- 大写宏：`RCEC`
- 外部类型：`Advisory`
- 外部类型：`Assume`
- 外部类型：`Bit`
- 外部类型：`Clear`
- 外部类型：`Collector`
- 外部类型：`Collectors`
- 外部类型：`Command`
- 外部类型：`Completion`
- 外部类型：`Complex`
- 外部类型：`Correctable`
- 外部类型：`Counters`
- 外部类型：`Disable`
- 外部类型：`Don`
- 外部类型：`Downstream`
- 外部类型：`Enable`
- 外部类型：`Endpoint`
- 外部类型：`Error`
- 外部类型：`Errors`
- 外部类型：`Event`
- 外部类型：`Fatal`
- 外部类型：`Fields`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Memory`
- 外部类型：`Non`
- 外部类型：`Note`
- 外部类型：`Only`
- 外部类型：`Others`
- 外部类型：`PCIe`
- 外部类型：`Port`
- 外部类型：`Ports`
- 外部类型：`Precalculate`
- 外部类型：`RCECs`
- 外部类型：`RCiEP`
- 外部类型：`Ratelimits`
- 外部类型：`Recursive`
- 外部类型：`Root`
- 外部类型：`Some`
- 外部类型：`Status`
- 外部类型：`That`
- 外部类型：`They`
- 外部类型：`Timeout`
- 外部类型：`Total`
- 外部类型：`Uncorrectable`
- 外部类型：`Unknown`
- 外部类型：`aer_capability_regs`
- 外部类型：`aer_err_info`
- 外部类型：`pci_dev`
- 外部类型：`pci_ers_result_t`
- 外部类型：`pci_host_bridge`
- 外部类型：`ratelimit_state`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-e437aa70d9` → 本草稿移入 `cases/defect/auto-linux-e437aa70d9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
