# auto-linux-51b6f2dee2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#44dc57ed1e77a23248ef28732486f4b7e9468cf7](https://github.com/torvalds/linux/commit/44dc57ed1e77a23248ef28732486f4b7e9468cf7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #44dc57ed1e77a23248ef28732486f4b7e9468cf7 (https://github.com/torvalds/linux/commit/44dc57ed1e77a23248ef28732486f4b7e9468cf7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 595；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 44dc57ed1e77a23248ef28732486f4b7e9468cf7 Merge tag 'sh-for-v7.4-tag1' of git://git.kernel.org/pub/scm/linux/kernel/git/glaubitz/sh-linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -26,6 +26,7 @@
 #include <linux/platform_data/tmio.h>
 #include <linux/platform_data/tsc2007.h>
 #include <linux/platform_device.h>
+#include <linux/property.h>
 #include <linux/regulator/fixed.h>
 #include <linux/regulator/machine.h>
 #include <linux/sh_eth.h>
@@ -589,39 +590,21 @@ static struct platform_device keysc_device = {
 /* TouchScreen */
 #define IRQ0 evt2irq(0x600)
 
-static int ts_get_pendown_state(struct device *dev)
-{
-	int val = 0;
-	gpio_free(GPIO_FN_INTC_IRQ0);
-	gpio_request(GPIO_PTZ0, NULL);
-	gpio_direction_input(GPIO_PTZ0);
-
-	val = gpio_get_value(GPIO_PTZ0);
-
-	gpio_free(GPIO_PTZ0);
-	gpio_request(GPIO_FN_INTC_IRQ0, NULL);
-
-	return val ? 0 : 1;
-}
-
-static int ts_init(void)
-{
-	gpio_request(GPIO_FN_INTC_IRQ0, NULL);
-	return 0;
-}
+static const struct property_entry tsc2007_properties[] = {
+	PROPERTY_ENTRY_U32("ti,x-plate-ohms", 180),
+	{ }
+};
 
-static struct tsc2007_platform_data tsc2007_info = {
-	.model			= 2007,
-	.x_plate_ohms		= 180,
-	.get_pendown_state	= ts_get_pendown_state,
-	.init_platform_hw	= ts_init,
+static const struct software_node tsc2007_swnode = {
+	.name = "tsc2007",
+	.properties = tsc2007_properties,
 };
 
 static struct i2c_board_info ts_i2c_clients = {
 	I2C_BOARD_INFO("tsc2007", 0x48),
 	.type		= "tsc2007",
-	.platform_data	= &tsc2007_info,
 	.irq		= IRQ0,
+	.swnode		= &tsc2007_swnode,
 };
 
 static struct regulator_consumer_supply cn12_power_consumers[] =
@@ -1242,8 +1225,9 @@ static int __init arch_setup(void)
 		gpio_direction_output(GPIO_PTF4, 1);
 
 		/* enable TouchScreen */
-		i2c_register_board_info(0, &ts_i2c_clients, 1);
+		gpio_request(GPIO_FN_INTC_IRQ0, NULL);
 		irq_set_irq_type(IRQ0, IRQ_TYPE_LEVEL_LOW);
+		i2c_register_board_info(0, &ts_i2c_clients, 1);
 	}
 
 	/* enable CEU0 */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`gpio_direction_input`
- 外部函数：`gpio_direction_output`
- 外部函数：`gpio_free`
- 外部函数：`gpio_get_value`
- 外部函数：`gpio_request`
- 外部函数：`i2c_register_board_info`
- 外部函数：`irq_set_irq_type`
- 外部函数：`ts_get_pendown_state`
- 大写宏：`CEU0`
- 大写宏：`GPIO_FN_INTC_IRQ0`
- 大写宏：`GPIO_PTF4`
- 大写宏：`GPIO_PTZ0`
- 大写宏：`I2C_BOARD_INFO`
- 大写宏：`IRQ_TYPE_LEVEL_LOW`
- 大写宏：`NULL`
- 外部类型：`TouchScreen`
- 外部类型：`device`
- 外部类型：`i2c_board_info`
- 外部类型：`regulator_consumer_supply`
- 外部类型：`tsc2007_platform_data`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-51b6f2dee2` → 本草稿移入 `cases/defect/auto-linux-51b6f2dee2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
