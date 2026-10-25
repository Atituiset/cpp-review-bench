# auto-linux-7a60e67faf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b](https://github.com/torvalds/linux/commit/9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b (https://github.com/torvalds/linux/commit/9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 513；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b Merge tag 'gpio-fixes-for-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/brgl/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -510,7 +510,7 @@ static int lpc32xx_of_xlate(struct gpio_chip *gc,
 
 static int lpc32xx_gpio_probe(struct platform_device *pdev)
 {
-	int i;
+	int i, ret;
 	void __iomem *reg_base;
 
 	reg_base = devm_platform_ioremap_resource(pdev, 0);
@@ -524,8 +524,11 @@ static int lpc32xx_gpio_probe(struct platform_device *pdev)
 			lpc32xx_gpiochip[i].chip.of_gpio_n_cells = 3;
 			lpc32xx_gpiochip[i].reg_base = reg_base;
 		}
-		devm_gpiochip_add_data(&pdev->dev, &lpc32xx_gpiochip[i].chip,
-				  &lpc32xx_gpiochip[i]);
+		ret = devm_gpiochip_add_data(&pdev->dev, &lpc32xx_gpiochip[i].chip,
+					     &lpc32xx_gpiochip[i]);
+		if (ret)
+			return dev_err_probe(&pdev->dev, ret,
+					"failed to register gpio chip %d\n", i);
 	}
 
 	return 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`devm_gpiochip_add_data`
- 外部函数：`devm_platform_ioremap_resource`
- 外部类型：`platform_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7a60e67faf` → 本草稿移入 `cases/defect/auto-linux-7a60e67faf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
