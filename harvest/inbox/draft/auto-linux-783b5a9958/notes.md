# auto-linux-783b5a9958

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb](https://github.com/torvalds/linux/commit/b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb (https://github.com/torvalds/linux/commit/b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1184；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb Merge tag 'tty-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/tty :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1181,7 +1181,7 @@ static int xr17v35x_register_gpio(struct exar8250 *priv, struct pci_dev *pcidev,
 
 static void xr17v35x_unregister_gpio(struct exar8250 *priv)
 {
-	if (priv->gpio) {
+	if (!IS_ERR_OR_NULL(priv->gpio)) {
 		__xr17v35x_unregister_gpio(priv->gpio);
 		priv->gpio = NULL;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`device_remove_software_node`
- 外部函数：`int`
- 外部函数：`platform_device_unregister`
- 外部函数：`void`
- 大写宏：`NULL`
- 外部类型：`eeprom_93cx6`
- 外部类型：`pci_dev`
- 外部类型：`platform_device`
- 外部类型：`uart_8250_port`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-783b5a9958` → 本草稿移入 `cases/defect/auto-linux-783b5a9958/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
