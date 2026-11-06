# auto-linux-30b4d66ec4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3e516eae690b3f24c19168cc0fe605e1c610eb21](https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 41 |
| 编译错误数（gcc syntax-only） | 65（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3e516eae690b3f24c19168cc0fe605e1c610eb21 (https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 397；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3e516eae690b3f24c19168cc0fe605e1c610eb21 Merge tag 'hwmon-for-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -30,13 +30,6 @@
 #define POWER_METER_CAN_CAP	(1 << 2)
 #define POWER_METER_CAN_NOTIFY	(1 << 3)
 #define POWER_METER_IS_BATTERY	(1 << 8)
-#define UNKNOWN_HYSTERESIS	0xFFFFFFFF
-/*
- * ACPI integers can either be 32-bit or 64-bit wide, so
- * we have to check for both values.
- */
-#define UNKNOWN_POWER_32	U32_MAX
-#define UNKNOWN_POWER_64	U64_MAX
 
 #define METER_NOTIFY_CONFIG	0x80
 #define METER_NOTIFY_TRIP	0x81
@@ -54,6 +47,15 @@ static bool force_cap_on;
 
 static DEFINE_MUTEX(acpi_notify_lock);
 
+/*
+ * ACPI integers can either be 32-bit or 64-bit wide, so
+ * we have to check for both values.
+ */
+static bool acpi_power_meter_is_unknown(u64 val)
+{
+	return val == U32_MAX || val == U64_MAX;
+}
+
 static int can_cap_in_hardware(void)
 {
 	return force_cap_on || cap_in_hardware;
@@ -179,9 +181,10 @@ static int update_meter(struct acpi_power_meter_resource *resource)
 	acpi_status status;
 	unsigned long local_jiffies = jiffies;
 
-	if (time_before(local_jiffies, resource->sensors_last_updated +
+	if (!acpi_power_meter_is_unknown(resource->caps.sampling_time) &&
+	    time_before(local_jiffies, resource->sensors_last_updated +
 			msecs_to_jiffies(resource->caps.sampling_time)) &&
-			resource->sensors_valid)
+	    resource->sensors_valid)
 		return 0;
 
 	status = acpi_evaluate_integer(resource->acpi_dev->handle, "_PMM",
@@ -377,28 +380,30 @@ static int set_avg_interval(struct acpi_power_meter_resource *resource,
 	return 0;
 }
 
-static int get_power_alarm_state(struct acpi_power_meter_resource *resource,
-				 long *val)
+static bool get_power_alarm_state(struct acpi_power_meter_resource *resource)
 {
+	bool alarm = false;
 	int ret;
 
+	if (resource->caps.flags & POWER_METER_CAN_NOTIFY)
+		alarm = resource->power_alarm;
+	resource->power_alarm = false;
+
 	ret = update_meter(resource);
-	if (ret)
-		return ret;
+	if (ret || acpi_power_meter_is_unknown(resource->power))
+		return alarm;
 
 	/* need to update cap if not to support the notification. */
 	if (!(resource->caps.flags & POWER_METER_CAN_NOTIFY)) {
 		ret = update_cap(resource);
 		if (ret)
-			return ret;
-		resource->power_alarm = resource->power > resource->cap;
-		*val = resource->power_alarm;
-	} else {
-		*val = resource->power_alarm || resource->power > resource->cap;
-		resource->power_alarm = resource->power > resource->cap;
+			return false;
 	}
 
-	return 0;
+	if (acpi_power_meter_is_unknown(resource->cap))
+		return alarm;
+
+	return alarm || resource->power > resource->cap;
 }
 
 static umode_t power_meter_is_visible(const void *data,
@@ -458,37 +463,47 @@ static int power_meter_read(struct device *dev, enum hwmon_sensor_types type,
 		ret = update_meter(res);
 		if (ret)
 			return ret;
-		if (res->power == UNKNOWN_POWER_32 || res->power == UNKNOWN_POWER_64)
+		if (acpi_power_meter_is_unknown(res->power))
 			return -ENODATA;
 		*val = res->power * 1000;
 		break;
 	case hwmon_power_average_interval_min:
+		if (acpi_power_meter_is_unknown(res->caps.min_avg_interval))
+			return -ENODATA;
 		*val = res->caps.min_avg_interval;
 		break;
 	case hwmon_power_average_interval_max:
+		if (acpi_power_meter_is_unknown(res->caps.max_avg_interval))
+			return -ENODATA;
 		*val = res->caps.max_avg_interval;
 		break;
 	case hwmon_power_average_interval:
 		ret = update_avg_interval(res);
 		if (ret)
 			return ret;
-		*val = (res)->avg_interval;
+		if (acpi_power_meter_is_unknown(res->avg_interval))
+			return -ENODATA;
+		*val = res->avg_interval;
 		break;
 	case hwmon_power_cap_min:
+		if (acpi_power_meter_is_unknown(res->caps.min_cap))
+			return -ENODATA;
 		*val = res->caps.min_cap * 1000;
 		break;
 	case hwmon_power_cap_max:
+		if (acpi_power_meter_is_unknown(res->caps.max_cap))
+			return -ENODATA;
 		*val = res->caps.max_cap * 1000;
 		break;
 	case hwmon_power_alarm:
-		ret = get_power_alarm_state(res, val);
-		if (ret)
-			return ret;
+		*val = get_power_alarm_state(res);
 		break;
 	case hwmon_power_cap:
 		ret = update_cap(res);
 		if (ret)
 	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acpi_evaluate_integer`
- 外部函数：`acpi_evaluation_failure_warn`
- 外部函数：`dev_get_drvdata`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`sysfs_emit`
- 外部函数：`time_before`
- 大写宏：`ACPI`
- 大写宏：`ACPI_FAILURE`
- 大写宏：`DEFINE_MUTEX`
- 大写宏：`ENODATA`
- 大写宏：`ENODEV`
- 大写宏：`NULL`
- 大写宏：`U32_MAX`
- 大写宏：`U64_MAX`
- 外部类型：`acpi_device`
- 外部类型：`acpi_power_meter_capabilities`
- 外部类型：`device`
- 外部类型：`hwmon_sensor_types`
- 外部类型：`kobject`
- 外部类型：`mutex`
- 外部类型：`umode_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-30b4d66ec4` → 本草稿移入 `cases/defect/auto-linux-30b4d66ec4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
