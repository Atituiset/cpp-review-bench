# auto-linux-129bded59e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3e516eae690b3f24c19168cc0fe605e1c610eb21](https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 131 |
| 编译错误数（gcc syntax-only） | 190（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3e516eae690b3f24c19168cc0fe605e1c610eb21 (https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1685；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3e516eae690b3f24c19168cc0fe605e1c610eb21 Merge tag 'hwmon-for-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1524,14 +1524,22 @@ static int pmbus_thermal_add_sensor(struct pmbus_data *pmbus_data,
 	tdata->sensor = sensor;
 	tdata->pmbus_data = pmbus_data;
 
-	tzd = devm_thermal_of_zone_register(dev, index, tdata,
+	/*
+	 * The PMBus core numbers its temperature sensors from 1, as the
+	 * hwmon tempN attributes, while the thermal core expects a 0-based
+	 * sensor index.
+	 */
+	tzd = devm_thermal_of_zone_register(dev, index - 1, tdata,
 					    &pmbus_thermal_ops);
 	/*
-	 * If CONFIG_THERMAL_OF is disabled, this returns -ENODEV,
-	 * so ignore that error but forward any other error.
+	 * If CONFIG_THERMAL_OF is disabled, devm_thermal_of_zone_register()
+	 * returns -ENOTSUPP. If no thermal zone is configured for the sensor,
+	 * it returns -ENODEV. Ignore those errors but forward any other error.
 	 */
-	if (IS_ERR(tzd) && (PTR_ERR(tzd) != -ENODEV))
+	if (IS_ERR(tzd) && PTR_ERR(tzd) !=  -ENODEV && PTR_ERR(tzd) != -ENOTSUPP) {
+		devm_kfree(dev, tdata);
 		return PTR_ERR(tzd);
+	}
 
 	return 0;
 }
@@ -1546,10 +1554,11 @@ static struct pmbus_sensor *pmbus_add_sensor(struct pmbus_data *data,
 {
 	struct sensor_device_attribute *a;
 	struct pmbus_sensor *sensor;
+	int err;
 
 	sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
 	if (!sensor)
-		return NULL;
+		return ERR_PTR(-ENOMEM);
 	a = &sensor->attribute;
 
 	if (type)
@@ -1574,14 +1583,17 @@ static struct pmbus_sensor *pmbus_add_sensor(struct pmbus_data *data,
 			pmbus_set_sensor, -1);
 
 	if (pmbus_add_attribute(data, &a->dev_attr.attr))
-		return NULL;
+		return ERR_PTR(-ENOMEM);
 
 	sensor->next = data->sensors;
 	data->sensors = sensor;
 
 	/* temperature sensors with _input values are registered with thermal */
-	if (class == PSC_TEMPERATURE && strcmp(type, "input") == 0)
-		pmbus_thermal_add_sensor(data, sensor, seq);
+	if (class == PSC_TEMPERATURE && strcmp(type, "input") == 0) {
+		err = pmbus_thermal_add_sensor(data, sensor, seq);
+		if (err)
+			return ERR_PTR(err);
+	}
 
 	return sensor;
 }
@@ -1682,8 +1694,8 @@ static int pmbus_add_limit_attrs(struct i2c_client *client,
 						page, 0xff, l->reg, attr->class,
 						attr->update || l->update,
 						l->readonly, l->writeonly, true);
-			if (!curr)
-				return -ENOMEM;
+			if (IS_ERR(curr))
+				return PTR_ERR(curr);
 			if (l->sbit && (info->func[page] & attr->sfunc)) {
 				ret = pmbus_add_boolean(data, name,
 					l->alarm, index,
@@ -1722,8 +1734,8 @@ static int pmbus_add_sensor_attrs_one(struct i2c_client *client,
 	}
 	base = pmbus_add_sensor(data, name, "input", index, page, phase,
 				attr->reg, attr->class, true, true, false, true);
-	if (!base)
-		return -ENOMEM;
+	if (IS_ERR(base))
+		return PTR_ERR(base);
 	/* No limit and alarm attributes for phase specific sensors */
 	if (attr->sfunc && phase == 0xff) {
 		ret = pmbus_add_limit_attrs(client, data, info, name,
@@ -2396,8 +2408,8 @@ static int pmbus_add_fan_ctrl(struct i2c_client *client,
 				  0xff, PMBUS_VIRT_FAN_TARGET_1 + id, PSC_FAN,
 				  false, false, false, true);
 
-	if (!sensor)
-		return -ENOMEM;
+	if (IS_ERR(sensor))
+		return PTR_ERR(sensor);
 
 	if (!((data->info->func[page] & PMBUS_HAVE_PWM12) ||
 	      (data->info->func[page] & PMBUS_HAVE_PWM34)))
@@ -2407,15 +2419,15 @@ static int pmbus_add_fan_ctrl(struct i2c_client *client,
 				  0xff, PMBUS_VIRT_PWM_1 + id, PSC_PWM,
 				  false, false, false, true);
 
-	if (!sensor)
-		return -ENOMEM;
+	if (IS_ERR(sensor))
+		return PTR_ERR(sensor);
 
 	sensor = pmbus_add_sensor(data, "pwm", "enable", index, page,
 				  0xff, PMBUS_VIRT_PWM_ENABLE_1 + id, PSC_PWM,
 				  true, false, false, false);
 
-	if (!sensor)
-		return -ENOMEM;
+	if (IS_ERR(sensor))
+		return PTR_ERR(sensor);
 
 	return 0;
 }
@@ -2432,6 +2444,7 @@ static int pmbus_add_fan_attributes(struct i2c_client *client,
 		int f;
 
 		for (f = 0; f < ARRAY_SIZE(pmbus_fan_registers); f++) {
+			struct pmbus_sensor *sensor;
 			int regval;
 
 			if (!(info->func[page] & pmbus_fan_flags[f]))
@@ -2452,10 +2465,11 @@ stat
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clamp_val`
- 外部函数：`container_of`
- 外部函数：`devm_krealloc_array`
- 外部函数：`devm_kzalloc`
- 外部函数：`devm_thermal_of_zone_register`
- 外部函数：`div_s64`
- 外部函数：`fsleep`
- 外部函数：`func`
- 外部函数：`guard`
- 外部函数：`i2c_get_clientdata`
- 外部函数：`i2c_smbus_read_byte_data`
- 外部函数：`i2c_smbus_read_word_data`
- 外部函数：`i2c_smbus_write_byte`
- 外部函数：`i2c_smbus_write_byte_data`
- 外部函数：`i2c_smbus_write_word_data`
- 外部函数：`int`
- 外部函数：`kstrtos64`
- 外部函数：`ktime_add_us`
- 外部函数：`ktime_get`
- 外部函数：`ktime_us_delta`
- 外部函数：`max`
- 外部函数：`mutex_lock`
- 外部函数：`pb_index_to_mask`
- 外部函数：`pb_index_to_page`
- 外部函数：`pb_index_to_reg`
- 外部函数：`pb_reg_to_index`
- 外部函数：`pmbus_get_fan_rate`
- 外部函数：`pmbus_lock`
- 外部函数：`read_byte_data`
- 外部函数：`read_status`
- 外部函数：`read_word_data`
- 外部函数：`snprintf`
- 外部函数：`ssize_t`
- 外部函数：`strcmp`
- 外部函数：`sysfs_attr_init`
- 外部函数：`sysfs_emit`
- 外部函数：`to_i2c_client`
- 外部函数：`to_pmbus_boolean`
- 外部函数：`to_pmbus_sensor`
- 外部函数：`to_sensor_dev_attr`
- 外部函数：`write_byte`
- 外部函数：`write_byte_data`
- 外部函数：`write_word_data`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`CONFIG_REGULATOR`
- 大写宏：`CONFIG_THERMAL_OF`
- 大写宏：`DIV_ROUND_CLOSEST`
- 大写宏：`DIV_ROUND_CLOSEST_ULL`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENODATA`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`ENXIO`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`LINEAR11`
- 大写宏：`LINEAR16`
- 大写宏：`NULL`
- 大写宏：`PB_CML_FAULT_INVALID_COMMAND`
- 大写宏：`PB_FAN_1_INSTALLED`
- 大写宏：`PB_STATUS_CML`
- 大写宏：`PMBUS_CLEAR_FAULTS`
- 大写宏：`PMBUS_HAVE_PWM12`
- 大写宏：`PMBUS_HAVE_PWM34`
- 大写宏：`PMBUS_OP_PAGE_CHANGE`
- 大写宏：`PMBUS_OP_WRITE`
- 大写宏：`PMBUS_PAGE`
- 大写宏：`PMBUS_PAGES`
- 大写宏：`PMBUS_PAGE_VIRTUAL`
- 大写宏：`PMBUS_PHASE`
- 大写宏：`PMBUS_PHASE_VIRTUAL`
- 大写宏：`PMBUS_READ_STATUS_AFTER_FAILED_CHECK`
- 大写宏：`PMBUS_REV_12`
- 大写宏：`PMBUS_SKIP_STATUS_CHECK`
- 大写宏：`PMBUS_STATUS_CML`
- 大写宏：`PMBUS_STATUS_WORD`
- 大写宏：`PMBUS_VIRT_BASE`
- 大写宏：`PMBUS_VIRT_FAN_TARGET_1`
- 大写宏：`PMBUS_VIRT_FAN_TARGET_4`
- 大写宏：`PMBUS_VIRT_PWM_1`
- 大写宏：`PMBUS_VIRT_PWM_ENABLE_1`
- 大写宏：`PMBUS_WRITE_PROTECTED`
- 大写宏：`PSC_FAN`
- 大写宏：`PSC_POWER`
- 大写宏：`PSC_PWM`
- 大写宏：`PSC_TEMPERATURE`
- 大写宏：`PSC_VOLTAGE_OUT`
- 大写宏：`PTR_ERR`
- 大写宏：`S16_MAX`
- 大写宏：`S16_MIN`
- 大写宏：`STATUS_WORD`
- 大写宏：`VID`
- 大写宏：`WARN`
- 外部类型：`Adjust`
- 外部类型：`Alarm`
- 外部类型：`Attribute`
- 外部类型：`Bad`
- 外部类型：`Calculate`
- 外部类型：`Convert`
- 外部类型：`Ensure`
- 外部类型：`Fan`
- 外部类型：`For`
- 外部类型：`Generic`
- 外部类型：`If`
- 外部类型：`Increase`
- 外部类型：`Limit`
- 外部类型：`NaN`
- 外部类型：`No`
- 外部类型：`PMBus`
- 外部类型：`Power`
- 外部类型：`Reduce`
- 外部类型：`Sensor`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`True`
- 外部类型：`Valid`
- 外部类型：`Wait`
- 外部类型：`Whether`
- 外部类型：`atomic_t`
- 外部类型：`attribute`
- 外部类型：`attribute_group`
- 外部类型：`device`
- 外部类型：`device_attribute`
- 外部类型：`i2c_client`
- 外部类型：`ktime_t`
- 外部类型：`mutex`
- 外部类型：`pmbus_driver_info`
- 外部类型：`pmbus_fan_mode`
- 外部类型：`pmbus_sensor_classes`
- 外部类型：`regulator_dev`
- 外部类型：`sensor_device_attribute`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`thermal_zone_device`
- 外部类型：`umode_t`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-129bded59e` → 本草稿移入 `cases/defect/auto-linux-129bded59e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
