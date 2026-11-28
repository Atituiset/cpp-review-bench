# auto-linux-4598acbc28

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ad5b48d7188721ba3a7a351d06a78f8030148e61](https://github.com/torvalds/linux/commit/ad5b48d7188721ba3a7a351d06a78f8030148e61) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ad5b48d7188721ba3a7a351d06a78f8030148e61 (https://github.com/torvalds/linux/commit/ad5b48d7188721ba3a7a351d06a78f8030148e61)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 47（原始 PR diff 行 246；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ad5b48d7188721ba3a7a351d06a78f8030148e61 Merge tag 'hwmon-for-v7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -136,9 +136,11 @@ struct ltc2978_data {
 
 #define FEAT_CLEAR_PEAKS	BIT(0)
 #define FEAT_NEEDS_POLLING	BIT(1)
+#define FEAT_IEEE754		BIT(2)
 
 #define has_clear_peaks(d)	((d)->features & FEAT_CLEAR_PEAKS)
 #define needs_polling(d)	((d)->features & FEAT_NEEDS_POLLING)
+#define has_ieee754(d)		((d)->features & FEAT_IEEE754)
 
 static int ltc_wait_ready(struct i2c_client *client)
 {
@@ -236,14 +238,38 @@ static inline int lin11_to_val(int data)
 	return (e < 0 ? m >> -e : m << e);
 }
 
+static inline int ieee754_sort_key(u16 h)
+{
+	int mag = h & 0x7fff;
+
+	return (h & 0x8000) ? -mag : mag;
+}
+
+static bool ltc_peak_gt(struct ltc2978_data *data, u16 a, u16 b)
+{
+	if (has_ieee754(data))
+		return ieee754_sort_key(a) > ieee754_sort_key(b);
+	return lin11_to_val(a) > lin11_to_val(b);
+}
+
+static u16 ltc_peak_max_init(struct ltc2978_data *data)
+{
+	return has_ieee754(data) ? 0xfc00 : 0x7c00;
+}
+
+static u16 ltc_peak_min_init(struct ltc2978_data *data)
+{
+	return has_ieee754(data) ? 0x7c00 : 0x7bff;
+}
+
 static int ltc_get_max(struct ltc2978_data *data, struct i2c_client *client,
 		       int page, int reg, u16 *pmax)
 {
 	int ret;
 
 	ret = ltc_read_word_data(client, page, 0xff, reg);
 	if (ret >= 0) {
-		if (lin11_to_val(ret) > lin11_to_val(*pmax))
+		if (ltc_peak_gt(data, ret, *pmax))
 			*pmax = ret;
 		ret = *pmax;
 	}
@@ -257,7 +283,7 @@ static int ltc_get_min(struct ltc2978_data *data, struct i2c_client *client,
 
 	ret = ltc_read_word_data(client, page, 0xff, reg);
 	if (ret >= 0) {
-		if (lin11_to_val(ret) < lin11_to_val(*pmin))
+		if (ltc_peak_gt(data, *pmin, ret))
 			*pmin = ret;
 		ret = *pmin;
 	}
@@ -494,22 +520,22 @@ static int ltc2978_write_word_data(struct i2c_client *client, int page,
 
 	switch (reg) {
 	case PMBUS_VIRT_RESET_IIN_HISTORY:
-		data->iin_max = 0x7c00;
-		data->iin_min = 0x7bff;
+		data->iin_max = ltc_peak_max_init(data);
+		data->iin_min = ltc_peak_min_init(data);
 		ret = ltc2978_clear_peaks(data, client, 0);
 		break;
 	case PMBUS_VIRT_RESET_PIN_HISTORY:
-		data->pin_max = 0x7c00;
-		data->pin_min = 0x7bff;
+		data->pin_max = ltc_peak_max_init(data);
+		data->pin_min = ltc_peak_min_init(data);
 		ret = ltc2978_clear_peaks(data, client, 0);
 		break;
 	case PMBUS_VIRT_RESET_IOUT_HISTORY:
-		data->iout_max[page] = 0x7c00;
-		data->iout_min[page] = 0xfbff;
+		data->iout_max[page] = ltc_peak_max_init(data);
+		data->iout_min[page] = has_ieee754(data) ? 0x7c00 : 0xfbff;
 		ret = ltc2978_clear_peaks(data, client, page);
 		break;
 	case PMBUS_VIRT_RESET_TEMP2_HISTORY:
-		data->temp2_max = 0x7c00;
+		data->temp2_max = ltc_peak_max_init(data);
 		ret = ltc2978_clear_peaks(data, client, page);
 		break;
 	case PMBUS_VIRT_RESET_VOUT_HISTORY:
@@ -518,13 +544,13 @@ static int ltc2978_write_word_data(struct i2c_client *client, int page,
 		ret = ltc2978_clear_peaks(data, client, page);
 		break;
 	case PMBUS_VIRT_RESET_VIN_HISTORY:
-		data->vin_min = 0x7bff;
-		data->vin_max = 0x7c00;
+		data->vin_min = ltc_peak_min_init(data);
+		data->vin_max = ltc_peak_max_init(data);
 		ret = ltc2978_clear_peaks(data, client, page);
 		break;
 	case PMBUS_VIRT_RESET_TEMP_HISTORY:
-		data->temp_min[page] = 0x7bff;
-		data->temp_max[page] = 0x7c00;
+		data->temp_min[page] = ltc_peak_min_init(data);
+		data->temp_max[page] = ltc_peak_max_init(data);
 		ret = ltc2978_clear_peaks(data, client, page);
 		break;
 	default:
@@ -761,7 +787,19 @@ static int ltc2978_probe(struct i2c_client *client)
 	switch (data->id) {
 	case lt7170:
 	case lt7171:
-		data->features |= FEAT_CLEAR_PEAKS | FEAT_NEEDS_POLLING;
+		data->features |= FEAT_CLEAR_PEAKS | FEAT_NEEDS_POLLING |
+				  FEAT_IEEE754;
+		data->vin_max = ltc_peak_max_init(data);
+		for (i = 0; i < ARRAY_SIZE(data->iout_max); i++)
+			data->iout_max[i] = ltc_peak_max_init(data);
+		for (i = 0; i < ARRAY_SIZE(data->temp_max); i++)
+			data->temp_max[i] = ltc_peak_max_init(data);
+		data->temp2_max = ltc_peak_max_init(data);
+		data->vin_min = ltc_peak_min_init(data);
+		fo
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`accuracy`
- 外部函数：`container_of`
- 外部函数：`has_clear_peaks`
- 外部函数：`ltc_wait_ready`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`needs_polling`
- 外部函数：`pmbus_get_driver_info`
- 外部函数：`pmbus_read_byte_data`
- 外部函数：`pmbus_read_word_data`
- 外部函数：`pmbus_write_byte`
- 外部函数：`time_before`
- 外部函数：`usleep_range`
- 大写宏：`BIT`
- 大写宏：`EBADMSG`
- 大写宏：`ENODATA`
- 大写宏：`ENXIO`
- 大写宏：`ETIMEDOUT`
- 大写宏：`LTC3883`
- 大写宏：`NACK`
- 大写宏：`PEC`
- 大写宏：`PMBUS`
- 大写宏：`PMBUS_VIRT_READ_IOUT_MAX`
- 大写宏：`PMBUS_VIRT_READ_TEMP2_MAX`
- 大写宏：`PMBUS_VIRT_READ_TEMP_MAX`
- 大写宏：`PMBUS_VIRT_READ_TEMP_MIN`
- 大写宏：`PMBUS_VIRT_READ_VIN_MAX`
- 大写宏：`PMBUS_VIRT_READ_VIN_MIN`
- 大写宏：`PMBUS_VIRT_READ_VOUT_MAX`
- 大写宏：`PMBUS_VIRT_READ_VOUT_MIN`
- 大写宏：`PMBUS_VIRT_RESET_IOUT_HISTORY`
- 大写宏：`PMBUS_VIRT_RESET_TEMP2_HISTORY`
- 大写宏：`PMBUS_VIRT_RESET_TEMP_HISTORY`
- 大写宏：`PMBUS_VIRT_RESET_VIN_HISTORY`
- 大写宏：`PMBUS_VIRT_RESET_VOUT_HISTORY`
- 大写宏：`VOUT`
- 外部类型：`Add`
- 外部类型：`Controllers`
- 外部类型：`Managers`
- 外部类型：`Modules`
- 外部类型：`i2c_client`
- 外部类型：`pmbus_driver_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4598acbc28` → 本草稿移入 `cases/defect/auto-linux-4598acbc28/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
