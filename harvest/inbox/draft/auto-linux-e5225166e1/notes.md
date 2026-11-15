# auto-linux-e5225166e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#54975c6523dc77862c25deeb5fa395990b8d3fc4](https://github.com/torvalds/linux/commit/54975c6523dc77862c25deeb5fa395990b8d3fc4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #54975c6523dc77862c25deeb5fa395990b8d3fc4 (https://github.com/torvalds/linux/commit/54975c6523dc77862c25deeb5fa395990b8d3fc4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 72；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 54975c6523dc77862c25deeb5fa395990b8d3fc4 Merge tag 'hwmon-for-v7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -27,8 +27,8 @@
 
 #define REG_TACH(ch)		(0x52 + (ch) * 2)
 #define REG_TEMP_INPUT(ch)	(0x56 + (ch) * 2)
-#define REG_TEMP_MAX(ch)	(0x06 + (ch) * 2)
-#define REG_TEMP_CRIT(ch)	(0x0A + (ch) * 2)
+#define REG_TEMP_MAX(ch)	((ch) ? 0x0C : 0x06)
+#define REG_TEMP_CRIT(ch)	((ch) ? 0x08 : 0x0A)
 
 #define TEMP11_FROM_REG(reg)	((reg) / 32 * 125)
 #define TEMP11_TO_REG(val)	(DIV_ROUND_CLOSEST(clamp_val((val), -128000, \
@@ -68,8 +68,9 @@ static const int max31760_pwm_freq[] = {33, 150, 1500, 25000};
 
 static int tach_to_rpm(u16 tach)
 {
+	/* The count is 0 until the first measurement */
 	if (tach == 0)
-		tach = 1;
+		return 0;
 
 	return 60 * 100000 / tach / 2;
 }
@@ -533,8 +534,9 @@ static int max31760_probe(struct i2c_client *client)
 
 	dev_set_drvdata(dev, state);
 
-	/* Set alert output to comparator mode */
-	ret = regmap_set_bits(state->regmap, REG_CR2, CR2_ALERTS);
+	/* Leave standby mode, set alert output to comparator mode */
+	ret = regmap_update_bits(state->regmap, REG_CR2, CR2_STBY | CR2_ALERTS,
+				 CR2_ALERTS);
 	if (ret)
 		return dev_err_probe(dev, ret, "cannot write register\n");
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err_probe`
- 外部函数：`dev_set_drvdata`
- 外部函数：`regmap_set_bits`
- 外部函数：`tach_to_rpm`
- 大写宏：`BIT`
- 大写宏：`DIV_ROUND_CLOSEST`
- 外部类型：`Set`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e5225166e1` → 本草稿移入 `cases/defect/auto-linux-e5225166e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
