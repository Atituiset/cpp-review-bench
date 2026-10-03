# auto-linux-09782cd638

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b4e7fc36e31f59b318d38afe621ac20650314d27](https://github.com/torvalds/linux/commit/b4e7fc36e31f59b318d38afe621ac20650314d27) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b4e7fc36e31f59b318d38afe621ac20650314d27 (https://github.com/torvalds/linux/commit/b4e7fc36e31f59b318d38afe621ac20650314d27)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 254；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b4e7fc36e31f59b318d38afe621ac20650314d27 Merge tag 'asoc-fix-v7.3-rc5' of https://git.kernel.org/pub/scm/linux/kernel/git/broonie/sound into for-linus :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -251,7 +251,7 @@ __setup("amd_smn_debugfs_enable", amd_smn_enable_dfs);
 static int __init amd_smn_init(void)
 {
 	u16 count, num_roots, roots_per_node, node, num_nodes;
-	struct pci_dev *root;
+	struct pci_dev *root __free(pci_dev_put) = NULL;
 
 	if (!cpu_feature_enabled(X86_FEATURE_ZEN))
 		return 0;
@@ -262,7 +262,6 @@ static int __init amd_smn_init(void)
 		return 0;
 
 	num_roots = 0;
-	root = NULL;
 	while ((root = get_next_root(root))) {
 		pci_dbg(root, "Reserving PCI config space\n");
 
@@ -299,14 +298,13 @@ static int __init amd_smn_init(void)
 
 	count = 0;
 	node = 0;
-	root = NULL;
 	while (node < num_nodes && (root = get_next_root(root))) {
 		/* Use one root for each node and skip the rest. */
 		if (count++ % roots_per_node)
 			continue;
 
 		pci_dbg(root, "is root for AMD node %u\n", node);
-		amd_roots[node++] = root;
+		amd_roots[node++] = pci_dev_get(root);
 	}
 
 	if (enable_dfs) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpu_feature_enabled`
- 外部函数：`pci_dbg`
- 外部函数：`pci_get_class`
- 大写宏：`AMD`
- 大写宏：`NULL`
- 大写宏：`PCI`
- 大写宏：`PCI_CLASS_BRIDGE_HOST`
- 大写宏：`PCI_VENDOR_ID_AMD`
- 大写宏：`PCI_VENDOR_ID_HYGON`
- 大写宏：`X86_FEATURE_ZEN`
- 外部类型：`Device`
- 外部类型：`Function`
- 外部类型：`Reserving`
- 外部类型：`Root`
- 外部类型：`Use`
- 外部类型：`pci_dev`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-09782cd638` → 本草稿移入 `cases/defect/auto-linux-09782cd638/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
