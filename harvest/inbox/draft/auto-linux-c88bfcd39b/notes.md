# auto-linux-c88bfcd39b

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
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fddfc3ec31799a932bb92f1b8a84cb3d1f963be9 (https://github.com/torvalds/linux/commit/fddfc3ec31799a932bb92f1b8a84cb3d1f963be9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 98；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fddfc3ec31799a932bb92f1b8a84cb3d1f963be9 Merge tag 'pci-v7.3-fixes-2' of git://git.kernel.org/pub/scm/linux/kernel/git/pci/pci :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -95,9 +95,13 @@ static int of_pci_prop_bus_range(struct pci_dev *pdev,
 				 struct of_changeset *ocs,
 				 struct device_node *np)
 {
-	u32 bus_range[] = { pdev->subordinate->busn_res.start,
-			    pdev->subordinate->busn_res.end };
+	u32 bus_range[2];
 
+	if (!pdev->subordinate)
+		return 0;
+
+	bus_range[0] = pdev->subordinate->busn_res.start;
+	bus_range[1] = pdev->subordinate->busn_res.end;
 	return of_changeset_add_prop_u32_array(ocs, np, "bus-range", bus_range,
 					       ARRAY_SIZE(bus_range));
 }
@@ -220,6 +224,9 @@ static int of_pci_prop_intr_map(struct pci_dev *pdev, struct of_changeset *ocs,
 	int ret;
 	u8 pin;
 
+	if (!pdev->subordinate)
+		return 0;
+
 	pnode = pci_device_to_OF_node(pdev->bus->self);
 	if (!pnode)
 		pnode = pci_bus_to_OF_node(pdev->bus);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`of_changeset_add_prop_u32_array`
- 外部函数：`pci_bus_to_OF_node`
- 外部函数：`pci_device_to_OF_node`
- 大写宏：`ARRAY_SIZE`
- 外部类型：`device_node`
- 外部类型：`of_changeset`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c88bfcd39b` → 本草稿移入 `cases/defect/auto-linux-c88bfcd39b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
