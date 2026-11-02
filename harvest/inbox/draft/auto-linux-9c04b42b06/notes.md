# auto-linux-9c04b42b06

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9b6f2eabc8371dea951c244f4aa9e013f36de9f8](https://github.com/torvalds/linux/commit/9b6f2eabc8371dea951c244f4aa9e013f36de9f8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 115 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9b6f2eabc8371dea951c244f4aa9e013f36de9f8 (https://github.com/torvalds/linux/commit/9b6f2eabc8371dea951c244f4aa9e013f36de9f8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 902；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9b6f2eabc8371dea951c244f4aa9e013f36de9f8 Merge tag 'rtc-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/abelloni/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1,7 +1,7 @@
 // SPDX-License-Identifier: GPL-2.0
 /*
- * A driver for the I2C members of the Abracon AB x8xx RTC family,
- * and compatible: AB 1805 and AB 0805
+ * A driver for the I2C and SPI members of the Abracon AB x8xx RTC family,
+ * and compatible: AB 1805, AB 0805, AB 1815 and AB 0815
  *
  * Copyright 2014-2015 Macq S.A.
  *
@@ -16,7 +16,9 @@
 #include <linux/kstrtox.h>
 #include <linux/module.h>
 #include <linux/of.h>
+#include <linux/regmap.h>
 #include <linux/rtc.h>
+#include <linux/spi/spi.h>
 #include <linux/watchdog.h>
 
 #define ABX8XX_REG_HTH		0x00
@@ -50,12 +52,18 @@
 #define ABX8XX_CTRL2_RSVD	BIT(5)
 
 #define ABX8XX_REG_IRQ		0x12
+#define ABX8XX_IRQ_EX1E		BIT(0)
+#define ABX8XX_IRQ_EX2E		BIT(1)
 #define ABX8XX_IRQ_AIE		BIT(2)
+#define ABX8XX_IRQ_TIE		BIT(3)
+#define ABX8XX_IRQ_BLIE		BIT(4)
 #define ABX8XX_IRQ_IM_1_4	(0x3 << 5)
 
 #define ABX8XX_REG_CD_TIMER_CTL	0x18
 
 #define ABX8XX_REG_OSC		0x1c
+#define ABX8XX_OSC_ACIE		BIT(0)
+#define ABX8XX_OSC_OFIE		BIT(1)
 #define ABX8XX_OSC_FOS		BIT(3)
 #define ABX8XX_OSC_BOS		BIT(4)
 #define ABX8XX_OSC_ACAL_512	BIT(5)
@@ -101,8 +109,8 @@
 
 static u8 trickle_resistors[] = {0, 3, 6, 11};
 
-enum abx80x_chip {AB0801, AB0803, AB0804, AB0805,
-	AB1801, AB1803, AB1804, AB1805, RV1805, ABX80X};
+enum abx80x_chip {AB0801, AB0803, AB0804, AB0805, AB0815,
+	AB1801, AB1803, AB1804, AB1805, AB1815, RV1805, ABX80X};
 
 struct abx80x_cap {
 	u16 pn;
@@ -115,61 +123,66 @@ static struct abx80x_cap abx80x_caps[] = {
 	[AB0803] = {.pn = 0x0803},
 	[AB0804] = {.pn = 0x0804, .has_tc = true, .has_wdog = true},
 	[AB0805] = {.pn = 0x0805, .has_tc = true, .has_wdog = true},
+	[AB0815] = {.pn = 0x0815, .has_tc = true, .has_wdog = true},
 	[AB1801] = {.pn = 0x1801},
 	[AB1803] = {.pn = 0x1803},
 	[AB1804] = {.pn = 0x1804, .has_tc = true, .has_wdog = true},
 	[AB1805] = {.pn = 0x1805, .has_tc = true, .has_wdog = true},
+	[AB1815] = {.pn = 0x1815, .has_tc = true, .has_wdog = true},
 	[RV1805] = {.pn = 0x1805, .has_tc = true, .has_wdog = true},
 	[ABX80X] = {.pn = 0}
 };
 
 struct abx80x_priv {
 	struct rtc_device *rtc;
-	struct i2c_client *client;
+	struct regmap *regmap;
 	struct watchdog_device wdog;
+	int irq;
 };
 
-static int abx80x_write_config_key(struct i2c_client *client, u8 key)
+static int abx80x_write_config_key(struct device *dev, u8 key)
 {
-	if (i2c_smbus_write_byte_data(client, ABX8XX_REG_CFG_KEY, key) < 0) {
-		dev_err(&client->dev, "Unable to write configuration key\n");
+	struct abx80x_priv *priv = dev_get_drvdata(dev);
+
+	if (regmap_write(priv->regmap, ABX8XX_REG_CFG_KEY, key) < 0) {
+		dev_err(dev, "Unable to write configuration key\n");
 		return -EIO;
 	}
 
 	return 0;
 }
 
-static int abx80x_is_rc_mode(struct i2c_client *client)
+static int abx80x_is_rc_mode(struct device *dev)
 {
-	int flags = 0;
+	struct abx80x_priv *priv = dev_get_drvdata(dev);
+	unsigned int flags = 0;
+	int err;
 
-	flags =  i2c_smbus_read_byte_data(client, ABX8XX_REG_OSS);
-	if (flags < 0) {
-		dev_err(&client->dev,
-			"Failed to read autocalibration attribute\n");
-		return flags;
+	err = regmap_read(priv->regmap, ABX8XX_REG_OSS, &flags);
+	if (err < 0) {
+		dev_err(dev, "Failed to read autocalibration attribute\n");
+		return err;
 	}
 
 	return (flags & ABX8XX_OSS_OMODE) ? 1 : 0;
 }
 
-static int abx80x_enable_trickle_charger(struct i2c_client *client,
-					 u8 trickle_cfg)
+static int abx80x_enable_trickle_charger(struct device *dev, u8 trickle_cfg)
 {
+	struct abx80x_priv *priv = dev_get_drvdata(dev);
 	int err;
 
 	/*
 	 * Write the configuration key register to enable access to the Trickle
 	 * register
 	 */
-	if (abx80x_write_config_key(client, ABX8XX_CFG_KEY_MISC) < 0)
+	if (abx80x_write_config_key(dev, ABX8XX_CFG_KEY_MISC) < 0)
 		return -EIO;
 
-	err = i2c_smbus_write_byte_data(client, ABX8XX_REG_TRICKLE,
-					ABX8XX_TRICKLE_CHARGE_ENABLE |
-					trickle_cfg);
+	err = regmap_write(priv->regmap, ABX8XX_REG_TRICKLE,
+			   ABX8XX_TRICKLE_CHARGE_ENABLE | trickle_cfg);
 	if (er
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bcd2bin`
- 外部函数：`bin2bcd`
- 外部函数：`clear_bit`
- 外部函数：`dev_alert`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dev_info`
- 外部函数：`devm_kzalloc`
- 外部函数：`devm_request_threaded_irq`
- 外部函数：`devm_rtc_allocate_device`
- 外部函数：`devm_rtc_nvmem_register`
- 外部函数：`devm_rtc_register_device`
- 外部函数：`devm_watchdog_register_device`
- 外部函数：`i2c_check_functionality`
- 外部函数：`i2c_get_clientdata`
- 外部函数：`i2c_get_match_data`
- 外部函数：`i2c_set_clientdata`
- 外部函数：`i2c_smbus_read_byte_data`
- 外部函数：`i2c_smbus_read_i2c_block_data`
- 外部函数：`i2c_smbus_write_byte_data`
- 外部函数：`i2c_smbus_write_i2c_block_data`
- 外部函数：`min`
- 外部函数：`min_t`
- 外部函数：`module_i2c_driver`
- 外部函数：`of_match_ptr`
- 外部函数：`put_user`
- 外部函数：`rtc_add_group`
- 外部函数：`strcmp`
- 外部函数：`strncmp`
- 外部函数：`to_i2c_client`
- 外部函数：`watchdog_get_drvdata`
- 外部函数：`watchdog_hw_running`
- 外部函数：`watchdog_set_drvdata`
- 大写宏：`AB0801`
- 大写宏：`AB0803`
- 大写宏：`AB0804`
- 大写宏：`AB0805`
- 大写宏：`AB1801`
- 大写宏：`AB1803`
- 大写宏：`AB1804`
- 大写宏：`AB1805`
- 大写宏：`ABX80X`
- 大写宏：`BIT`
- 大写宏：`CONFIG_OF`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENODEV`
- 大写宏：`ENOIOCTLCMD`
- 大写宏：`ENOMEM`
- 大写宏：`EXTI`
- 大写宏：`FIELD_GET`
- 大写宏：`FIELD_PREP`
- 大写宏：`GENMASK`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GPL`
- 大写宏：`I2C`
- 大写宏：`I2C_FUNC_I2C`
- 大写宏：`I2C_SMBUS_BLOCK_MAX`
- 大写宏：`IRQ`
- 大写宏：`IRQF_ONESHOT`
- 大写宏：`IRQF_SHARED`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IRQ_NONE`
- 大写宏：`IS_ERR`
- 大写宏：`MODULE_AUTHOR`
- 大写宏：`MODULE_DEVICE_TABLE`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`RTC`
- 大写宏：`RTC_FEATURE_ALARM`
- 大写宏：`RTC_VL_BACKUP_LOW`
- 大写宏：`RTC_VL_CLR`
- 大写宏：`RTC_VL_READ`
- 大写宏：`RV1805`
- 大写宏：`SPDX`
- 大写宏：`WDT`
- 外部类型：`Abracon`
- 外部类型：`Alexandre`
- 外部类型：`Avoid`
- 外部类型：`Belloni`
- 外部类型：`Clear`
- 外部类型：`Control`
- 外部类型：`Control2`
- 外部类型：`Copyright`
- 外部类型：`De`
- 外部类型：`Disable`
- 外部类型：`Enabling`
- 外部类型：`Failed`
- 外部类型：`Failure`
- 外部类型：`Identifier`
- 外部类型：`Invalid`
- 外部类型：`License`
- 外部类型：`Macq`
- 外部类型：`Muyter`
- 外部类型：`Oscillator`
- 外部类型：`Philippe`
- 外部类型：`Read`
- 外部类型：`Register`
- 外部类型：`Status`
- 外部类型：`The`
- 外部类型：`Trickle`
- 外部类型：`Unable`
- 外部类型：`Unknown`
- 外部类型：`Unlock`
- 外部类型：`Write`
- 外部类型：`Writing`
- 外部类型：`device`
- 外部类型：`device_attribute`
- 外部类型：`device_node`
- 外部类型：`i2c_client`
- 外部类型：`i2c_device_id`
- 外部类型：`i2c_driver`
- 外部类型：`irqreturn_t`
- 外部类型：`of_device_id`
- 外部类型：`rtc_device`
- 外部类型：`rtc_time`
- 外部类型：`rtc_wkalrm`
- 外部类型：`size_t`
- 外部类型：`uintptr_t`
- 外部类型：`watchdog_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-9c04b42b06` → 本草稿移入 `cases/defect/auto-linux-9c04b42b06/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
