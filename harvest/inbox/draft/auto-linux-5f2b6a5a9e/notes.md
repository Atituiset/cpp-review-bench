# auto-linux-5f2b6a5a9e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#06ef54bf3ab7ab8531e8f9c197d189f44b8bd925](https://github.com/torvalds/linux/commit/06ef54bf3ab7ab8531e8f9c197d189f44b8bd925) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #06ef54bf3ab7ab8531e8f9c197d189f44b8bd925 (https://github.com/torvalds/linux/commit/06ef54bf3ab7ab8531e8f9c197d189f44b8bd925)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1458；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 06ef54bf3ab7ab8531e8f9c197d189f44b8bd925 Merge tag 'hwmon-for-v7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1455,10 +1455,10 @@ static int nct6775_update_pwm(struct device *dev)
 
 		if (!data->temp_tolerance[0][i] ||
 		    data->pwm_enable[i] != speed_cruise)
-			data->temp_tolerance[0][i] = fanmodecfg & 0x0f;
+			data->temp_tolerance[0][i] = fanmodecfg & data->tolerance_mask;
 		if (!data->target_speed_tolerance[i] ||
 		    data->pwm_enable[i] == speed_cruise) {
-			u8 t = fanmodecfg & 0x0f;
+			u8 t = fanmodecfg & data->tolerance_mask;
 
 			if (data->REG_TOLERANCE_H) {
 				err = nct6775_read_value(data, data->REG_TOLERANCE_H[i], &reg);
@@ -1857,7 +1857,7 @@ EXPORT_SYMBOL_GPL(nct6775_show_beep);
 ssize_t
 nct6775_store_beep(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
 {
-	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
+	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
 	struct nct6775_data *data = dev_get_drvdata(dev);
 	int nr = data->BEEP_BITS[sattr->index];
 	int regindex = nr >> 3;
@@ -1911,7 +1911,7 @@ static ssize_t
 store_temp_beep(struct device *dev, struct device_attribute *attr,
 		const char *buf, size_t count)
 {
-	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
+	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
 	struct nct6775_data *data = dev_get_drvdata(dev);
 	int nr, bit, regindex;
 	unsigned long val;
@@ -2997,7 +2997,7 @@ store_temp_tolerance(struct device *dev, struct device_attribute *attr,
 
 	mutex_lock(&data->update_lock);
 	data->temp_tolerance[index][nr] = val;
-	if (index)
+	if (!index)
 		err = pwm_update_registers(data, nr);
 	else
 		err = nct6775_write_value(data, data->REG_CRITICAL_TEMP_TOLERANCE[nr], val);
@@ -3572,6 +3572,9 @@ int nct6775_probe(struct device *dev, struct nct6775_data *data,
 
 		data->fan_from_reg = fan_from_reg13;
 		data->fan_from_reg_min = fan_from_reg13;
+		data->target_temp_mask = 0xff;
+		data->tolerance_mask = 0x07;
+		data->speed_tolerance_limit = 63;
 
 		data->temp_label = nct6776_temp_label;
 		data->temp_mask = NCT6776_TEMP_MASK;
@@ -3648,6 +3651,9 @@ int nct6775_probe(struct device *dev, struct nct6775_data *data,
 
 		data->fan_from_reg = fan_from_reg13;
 		data->fan_from_reg_min = fan_from_reg13;
+		data->target_temp_mask = 0xff;
+		data->tolerance_mask = 0x07;
+		data->speed_tolerance_limit = 63;
 
 		data->temp_label = nct6776_temp_label;
 		data->temp_mask = NCT6776_TEMP_MASK;
@@ -3725,6 +3731,9 @@ int nct6775_probe(struct device *dev, struct nct6775_data *data,
 
 		data->fan_from_reg = fan_from_reg13;
 		data->fan_from_reg_min = fan_from_reg13;
+		data->target_temp_mask = 0xff;
+		data->tolerance_mask = 0x07;
+		data->speed_tolerance_limit = 63;
 
 		data->temp_label = nct6776_temp_label;
 		data->temp_mask = NCT6776_TEMP_MASK;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_get_drvdata`
- 外部函数：`mutex_lock`
- 外部函数：`nct6775_read_value`
- 外部函数：`nct6775_write_value`
- 外部函数：`to_sensor_dev_attr_2`
- 大写宏：`BEEP_BITS`
- 大写宏：`REG_CRITICAL_TEMP_TOLERANCE`
- 大写宏：`REG_FAN_MODE`
- 大写宏：`REG_TARGET`
- 大写宏：`REG_TOLERANCE_H`
- 外部类型：`device`
- 外部类型：`device_attribute`
- 外部类型：`nct6775_data`
- 外部类型：`sensor_device_attribute_2`
- 外部类型：`size_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-5f2b6a5a9e` → 本草稿移入 `cases/defect/auto-linux-5f2b6a5a9e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
