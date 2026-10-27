# auto-linux-53a09b06dd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#d857e1f3faf998f2614dcdb52a90a27bc83bd71d](https://github.com/torvalds/linux/commit/d857e1f3faf998f2614dcdb52a90a27bc83bd71d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 41 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d857e1f3faf998f2614dcdb52a90a27bc83bd71d (https://github.com/torvalds/linux/commit/d857e1f3faf998f2614dcdb52a90a27bc83bd71d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 845；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d857e1f3faf998f2614dcdb52a90a27bc83bd71d Merge tag 'mfd-next-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/lee/mfd :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -18,14 +18,17 @@
 #include <linux/mutex.h>
 #include <linux/mfd/core.h>
 #include <linux/platform_device.h>
+#include <linux/property.h>
+#include <linux/regmap.h>
 #include <linux/slab.h>
 #include <linux/uaccess.h>
+#include <linux/units.h>
 
 #include <linux/mfd/lm3533.h>
 
 
-#define LM3533_ALS_RESISTOR_MIN			1
-#define LM3533_ALS_RESISTOR_MAX			127
+#define LM3533_ALS_RESISTOR_MIN_OHMS		1575
+#define LM3533_ALS_RESISTOR_MAX_OHMS		200000
 #define LM3533_ALS_CHANNEL_CURRENT_MAX		2
 #define LM3533_ALS_THRESH_MAX			3
 #define LM3533_ALS_ZONE_MAX			4
@@ -49,14 +52,17 @@
 
 
 struct lm3533_als {
-	struct lm3533 *lm3533;
+	struct regmap *regmap;
 	struct platform_device *pdev;
 
 	unsigned long flags;
 	int irq;
 
 	atomic_t zone;
 	struct mutex thresh_mutex;
+
+	u32 r_select;
+	bool pwm_mode;
 };
 
 
@@ -65,15 +71,15 @@ static int lm3533_als_get_adc(struct iio_dev *indio_dev, bool average,
 {
 	struct lm3533_als *als = iio_priv(indio_dev);
 	u8 reg;
-	u8 val;
+	u32 val;
 	int ret;
 
 	if (average)
 		reg = LM3533_REG_ALS_READ_ADC_AVERAGE;
 	else
 		reg = LM3533_REG_ALS_READ_ADC_RAW;
 
-	ret = lm3533_read(als->lm3533, reg, &val);
+	ret = regmap_read(als->regmap, reg, &val);
 	if (ret) {
 		dev_err(&indio_dev->dev, "failed to read adc\n");
 		return ret;
@@ -87,10 +93,10 @@ static int lm3533_als_get_adc(struct iio_dev *indio_dev, bool average,
 static int _lm3533_als_get_zone(struct iio_dev *indio_dev, u8 *zone)
 {
 	struct lm3533_als *als = iio_priv(indio_dev);
-	u8 val;
+	u32 val;
 	int ret;
 
-	ret = lm3533_read(als->lm3533, LM3533_REG_ALS_ZONE_INFO, &val);
+	ret = regmap_read(als->regmap, LM3533_REG_ALS_ZONE_INFO, &val);
 	if (ret) {
 		dev_err(&indio_dev->dev, "failed to read zone\n");
 		return ret;
@@ -128,7 +134,7 @@ static inline u8 lm3533_als_get_target_reg(unsigned channel, unsigned zone)
 }
 
 static int lm3533_als_get_target(struct iio_dev *indio_dev, unsigned channel,
-							unsigned zone, u8 *val)
+				 unsigned zone, u32 *val)
 {
 	struct lm3533_als *als = iio_priv(indio_dev);
 	u8 reg;
@@ -141,7 +147,7 @@ static int lm3533_als_get_target(struct iio_dev *indio_dev, unsigned channel,
 		return -EINVAL;
 
 	reg = lm3533_als_get_target_reg(channel, zone);
-	ret = lm3533_read(als->lm3533, reg, val);
+	ret = regmap_read(als->regmap, reg, val);
 	if (ret)
 		dev_err(&indio_dev->dev, "failed to get target current\n");
 
@@ -162,7 +168,7 @@ static int lm3533_als_set_target(struct iio_dev *indio_dev, unsigned channel,
 		return -EINVAL;
 
 	reg = lm3533_als_get_target_reg(channel, zone);
-	ret = lm3533_write(als->lm3533, reg, val);
+	ret = regmap_write(als->regmap, reg, val);
 	if (ret)
 		dev_err(&indio_dev->dev, "failed to set target current\n");
 
@@ -172,8 +178,8 @@ static int lm3533_als_set_target(struct iio_dev *indio_dev, unsigned channel,
 static int lm3533_als_get_current(struct iio_dev *indio_dev, unsigned channel,
 								int *val)
 {
+	u32 target;
 	u8 zone;
-	u8 target;
 	int ret;
 
 	ret = lm3533_als_get_zone(indio_dev, &zone);
@@ -272,16 +278,10 @@ static irqreturn_t lm3533_als_isr(int irq, void *dev_id)
 static int lm3533_als_set_int_mode(struct iio_dev *indio_dev, int enable)
 {
 	struct lm3533_als *als = iio_priv(indio_dev);
-	u8 mask = LM3533_ALS_INT_ENABLE_MASK;
-	u8 val;
 	int ret;
 
-	if (enable)
-		val = mask;
-	else
-		val = 0;
-
-	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_ZONE_INFO, val, mask);
+	ret = regmap_assign_bits(als->regmap, LM3533_REG_ALS_ZONE_INFO,
+				 LM3533_ALS_INT_ENABLE_MASK, enable);
 	if (ret) {
 		dev_err(&indio_dev->dev, "failed to set int mode %d\n",
 								enable);
@@ -294,17 +294,16 @@ static int lm3533_als_set_int_mode(struct iio_dev *indio_dev, int enable)
 static int lm3533_als_get_int_mode(struct iio_dev *indio_dev, int *enable)
 {
 	struct lm3533_als *als = iio_priv(indio_dev);
-	u8 mask = LM3533_ALS_INT_ENABLE_MASK;
-	u8 val;
 	int ret;
 
-	ret = lm3533_read(als->lm3533, LM3533_REG_ALS_ZONE_INFO, &val);
-	if (ret) {
+	ret = regmap_test_bits(als->regmap, LM
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_read`
- 外部函数：`atomic_set`
- 外部函数：`container_of`
- 外部函数：`dev_err`
- 外部函数：`dev_get_platdata`
- 外部函数：`dev_name`
- 外部函数：`dev_to_iio_dev`
- 外部函数：`devm_iio_device_alloc`
- 外部函数：`free_irq`
- 外部函数：`iio_device_set_parent`
- 外部函数：`iio_device_unregister`
- 外部函数：`iio_priv`
- 外部函数：`kstrtoul`
- 外部函数：`lm3533_read`
- 外部函数：`lm3533_update`
- 外部函数：`lm3533_write`
- 外部函数：`mutex_init`
- 外部函数：`mutex_lock`
- 外部函数：`platform_get_drvdata`
- 外部函数：`platform_set_drvdata`
- 外部函数：`test_bit`
- 大写宏：`ALS`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`EBUSY`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`INDIO_DIRECT_MODE`
- 大写宏：`LM3533_ATTR_TYPE_HYSTERESIS`
- 大写宏：`LM3533_ATTR_TYPE_TARGET`
- 大写宏：`LM3533_ATTR_TYPE_THRESH_FALLING`
- 大写宏：`LM3533_ATTR_TYPE_THRESH_RAISING`
- 大写宏：`PWM`
- 外部类型：`Make`
- 外部类型：`atomic_t`
- 外部类型：`device_attribute`
- 外部类型：`iio_dev`
- 外部类型：`lm3533`
- 外部类型：`lm3533_als_platform_data`
- 外部类型：`mutex`
- 外部类型：`platform_device`
- 外部类型：`platform_driver`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-53a09b06dd` → 本草稿移入 `cases/defect/auto-linux-53a09b06dd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
