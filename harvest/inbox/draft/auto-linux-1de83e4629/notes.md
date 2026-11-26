# auto-linux-1de83e4629

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ebb7a8dc1e74c705daffe11c2faf38ec2628f838](https://github.com/torvalds/linux/commit/ebb7a8dc1e74c705daffe11c2faf38ec2628f838) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 83 |
| 编译错误数（gcc syntax-only） | 81（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ebb7a8dc1e74c705daffe11c2faf38ec2628f838 (https://github.com/torvalds/linux/commit/ebb7a8dc1e74c705daffe11c2faf38ec2628f838)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 555；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ebb7a8dc1e74c705daffe11c2faf38ec2628f838 Merge tag 'soc-fixes-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/soc/soc :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -36,6 +36,12 @@ struct scmi_requested_dev {
 /* Track globally the SCMI SystemPower protocol device. */
 static struct scmi_device *scmi_syspower_registered;
 
+static bool scmi_raw_mode_only(void)
+{
+	return IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT) &&
+	       !IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT_COEX);
+}
+
 /**
  * scmi_protocol_device_request  - Helper to request a device
  *
@@ -60,8 +66,7 @@ static int scmi_protocol_device_request(const struct scmi_device_id *id_table)
 	pr_debug("Requesting SCMI device (%s) for protocol %x\n",
 		 id_table->name, id_table->protocol_id);
 
-	if (IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT) &&
-	    !IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT_COEX)) {
+	if (scmi_raw_mode_only()) {
 		pr_warn("SCMI Raw mode active. Rejecting '%s'/0x%02X\n",
 			id_table->name, id_table->protocol_id);
 		return -EINVAL;
@@ -181,12 +186,43 @@ static void scmi_protocol_device_unrequest(const struct scmi_device_id *id_table
 	}
 }
 
+/* Standard protocols table */
+static const struct scmi_device_id scmi_std_id_table[] = {
+	{ SCMI_PROTOCOL_POWER, "genpd" },
+	{ SCMI_PROTOCOL_SYSTEM, "syspower" },
+	{ SCMI_PROTOCOL_PERF, "perf" },
+	{ SCMI_PROTOCOL_PERF, "cpufreq" },
+	{ SCMI_PROTOCOL_CLOCK, "clocks" },
+	{ SCMI_PROTOCOL_SENSOR, "hwmon" },
+	{ SCMI_PROTOCOL_SENSOR, "iiodev" },
+	{ SCMI_PROTOCOL_RESET, "reset" },
+	{ SCMI_PROTOCOL_VOLTAGE, "regulator" },
+	{ SCMI_PROTOCOL_POWERCAP, "powercap" },
+	{ SCMI_PROTOCOL_PINCTRL, "pinctrl" },
+	{ },
+};
+
+static bool scmi_device_id_in_std_id_table(const struct scmi_device_id *id)
+{
+	for (int i = 0; scmi_std_id_table[i].name[0]; i++) {
+		if (scmi_std_id_table[i].protocol_id == id->protocol_id &&
+		    !strcmp(scmi_std_id_table[i].name, id->name))
+			return true;
+	}
+
+	return false;
+}
+
 static int scmi_protocol_table_register(const struct scmi_device_id *id_table)
 {
 	const struct scmi_device_id *entry;
 	int ret;
 
 	for (entry = id_table; entry->name[0]; entry++) {
+		/* Skip standard devices as they are created unconditionally */
+		if (scmi_device_id_in_std_id_table(entry))
+			continue;
+
 		ret = scmi_protocol_device_request(entry);
 		if (ret)
 			goto err_unrequest;
@@ -195,8 +231,11 @@ static int scmi_protocol_table_register(const struct scmi_device_id *id_table)
 	return 0;
 
 err_unrequest:
-	while (entry != id_table)
-		scmi_protocol_device_unrequest(--entry);
+	while (entry != id_table) {
+		--entry;
+		if (!scmi_device_id_in_std_id_table(entry))
+			scmi_protocol_device_unrequest(entry);
+	}
 
 	return ret;
 }
@@ -206,16 +245,23 @@ scmi_protocol_table_unregister(const struct scmi_device_id *id_table)
 {
 	const struct scmi_device_id *entry;
 
-	for (entry = id_table; entry->name[0]; entry++)
-		scmi_protocol_device_unrequest(entry);
+	for (entry = id_table; entry->name[0]; entry++) {
+		if (!scmi_device_id_in_std_id_table(entry))
+			scmi_protocol_device_unrequest(entry);
+	}
 }
 
-static bool scmi_device_is_transport(const struct scmi_device *scmi_dev)
+static bool scmi_device_name_is_transport(const char *name)
 {
-	return !strncmp(scmi_dev->name, SCMI_TRANSPORT_DEVNAME_PREFIX,
+	return !strncmp(name, SCMI_TRANSPORT_DEVNAME_PREFIX,
 			strlen(SCMI_TRANSPORT_DEVNAME_PREFIX));
 }
 
+static bool scmi_device_is_transport(const struct scmi_device *scmi_dev)
+{
+	return scmi_device_name_is_transport(scmi_dev->name);
+}
+
 static int __scmi_dev_match_by_id_table(struct scmi_device *scmi_dev,
 					const struct scmi_device_id *id_table,
 					bool skip_transport)
@@ -524,6 +570,11 @@ _scmi_device_create(struct fwnode_handle *fwnode, struct device *parent,
 {
 	struct scmi_device *sdev;
 
+	/* The perf device is only needed when it provides power domains. */
+	if (protocol == SCMI_PROTOCOL_PERF && !strcmp(name, "perf") &&
+	    !fwnode_property_present(fwnode, "#power-domain-cells"))
+		return NULL;
+
 	sdev = __scmi_device_create(fwnode, parent, protocol, name);
 	if (!sdev)
 		pr_err("(%pfwf) Failed to create device - protocol 0x%x (%
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`blocking_notifier_call_chain`
- 外部函数：`cmpxchg`
- 外部函数：`dev_fwnode`
- 外部函数：`dev_name`
- 外部函数：`dev_set_name`
- 外部函数：`dev_warn`
- 外部函数：`device`
- 外部函数：`device_find_child`
- 外部函数：`device_register`
- 外部函数：`device_set_node`
- 外部函数：`fwnode_handle_get`
- 外部函数：`fwnode_handle_put`
- 外部函数：`ida_alloc_min`
- 外部函数：`ida_free`
- 外部函数：`idr_alloc`
- 外部函数：`idr_find`
- 外部函数：`idr_remove`
- 外部函数：`kfree`
- 外部函数：`kfree_const`
- 外部函数：`kstrdup_const`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`pr_warn`
- 外部函数：`put_device`
- 外部函数：`scmi_match_by_id_table`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strscpy`
- 外部函数：`to_scmi_dev`
- 外部函数：`x`
- 大写宏：`CONFIG_ARM_SCMI_RAW_MODE_SUPPORT`
- 大写宏：`CONFIG_ARM_SCMI_RAW_MODE_SUPPORT_COEX`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`IS_ENABLED`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`ONLY`
- 大写宏：`SCMI`
- 大写宏：`SCMI_BUS_NOTIFY_DEVICE_REQUEST`
- 大写宏：`SCMI_BUS_NOTIFY_DEVICE_UNREQUEST`
- 大写宏：`SCMI_PROTOCOL_CLOCK`
- 大写宏：`SCMI_PROTOCOL_PERF`
- 大写宏：`SCMI_PROTOCOL_PINCTRL`
- 大写宏：`SCMI_PROTOCOL_POWER`
- 大写宏：`SCMI_PROTOCOL_POWERCAP`
- 大写宏：`SCMI_PROTOCOL_RESET`
- 大写宏：`SCMI_PROTOCOL_SENSOR`
- 大写宏：`SCMI_PROTOCOL_SYSTEM`
- 大写宏：`SCMI_PROTOCOL_VOLTAGE`
- 大写宏：`SCMI_TRANSPORT_DEVNAME_PREFIX`
- 外部类型：`Always`
- 外部类型：`Append`
- 外部类型：`Created`
- 外部类型：`Failed`
- 外部类型：`Find`
- 外部类型：`Helper`
- 外部类型：`If`
- 外部类型：`Ignoring`
- 外部类型：`No`
- 外部类型：`Nothing`
- 外部类型：`Raw`
- 外部类型：`Rejecting`
- 外部类型：`Requesting`
- 外部类型：`Reserve`
- 外部类型：`Return`
- 外部类型：`Standard`
- 外部类型：`SystemPower`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Track`
- 外部类型：`Unrequesting`
- 外部类型：`Walk`
- 外部类型：`device`
- 外部类型：`fwnode_handle`
- 外部类型：`list_head`
- 外部类型：`scmi_device`
- 外部类型：`scmi_device_id`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1de83e4629` → 本草稿移入 `cases/defect/auto-linux-1de83e4629/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
