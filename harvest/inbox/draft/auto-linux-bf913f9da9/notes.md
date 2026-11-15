# auto-linux-bf913f9da9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#23c62304dde29abe92013d85910d03a77dada35d](https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23c62304dde29abe92013d85910d03a77dada35d (https://github.com/torvalds/linux/commit/23c62304dde29abe92013d85910d03a77dada35d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 13（原始 PR diff 行 118；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23c62304dde29abe92013d85910d03a77dada35d Merge tag 'for-linus-7.4a-rc3-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/xen/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -95,6 +95,7 @@ static int platform_pci_probe(struct pci_dev *pdev,
 			      const struct pci_device_id *ent)
 {
 	int i, ret;
+	int requested_bars = 0;
 	long ioaddr;
 	long mmio_addr, mmio_len;
 	unsigned int max_nr_gframes;
@@ -115,16 +116,18 @@ static int platform_pci_probe(struct pci_dev *pdev,
 	if (mmio_addr == 0 || ioaddr == 0) {
 		dev_err(&pdev->dev, "no resources found\n");
 		ret = -ENOENT;
-		goto pci_out;
+		goto out;
 	}
 
 	ret = pci_request_region(pdev, 1, DRV_NAME);
 	if (ret < 0)
-		goto pci_out;
+		goto out;
+	requested_bars |= BIT(1);
 
 	ret = pci_request_region(pdev, 0, DRV_NAME);
 	if (ret < 0)
-		goto mem_out;
+		goto out;
+	requested_bars |= BIT(0);
 
 	platform_mmio = mmio_addr;
 	platform_mmiolen = mmio_len;
@@ -165,11 +168,8 @@ static int platform_pci_probe(struct pci_dev *pdev,
 	if (!xen_have_vector_callback)
 		free_irq(pdev->irq, pdev);
 out:
-	pci_release_region(pdev, 0);
-mem_out:
-	pci_release_region(pdev, 1);
-pci_out:
 	pci_disable_device(pdev);
+	pci_release_selected_regions(pdev, requested_bars);
 	return ret;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err`
- 外部函数：`free_irq`
- 外部函数：`pci_disable_device`
- 外部函数：`pci_release_region`
- 外部函数：`pci_request_region`
- 大写宏：`ENOENT`
- 外部类型：`pci_device_id`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-bf913f9da9` → 本草稿移入 `cases/defect/auto-linux-bf913f9da9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
