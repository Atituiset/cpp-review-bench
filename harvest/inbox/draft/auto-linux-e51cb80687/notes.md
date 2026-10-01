# auto-linux-e51cb80687

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#551c722f40809618230001baccf219193e22fc5a](https://github.com/torvalds/linux/commit/551c722f40809618230001baccf219193e22fc5a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #551c722f40809618230001baccf219193e22fc5a (https://github.com/torvalds/linux/commit/551c722f40809618230001baccf219193e22fc5a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 149；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 551c722f40809618230001baccf219193e22fc5a Merge tag 'rtc-7.3-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/abelloni/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -52,6 +52,7 @@ compute_wday(efi_time_t *eft, int yday)
 static void
 convert_to_efi_time(struct rtc_time *wtime, efi_time_t *eft)
 {
+	memset(eft, 0, sizeof(*eft));
 	eft->year	= wtime->tm_year + 1900;
 	eft->month	= wtime->tm_mon + 1;
 	eft->day	= wtime->tm_mday;
@@ -112,6 +113,35 @@ convert_from_efi_time(efi_time_t *eft, struct rtc_time *wtime)
 	return true;
 }
 
+static int efi_read_alarm(struct device *dev, struct rtc_wkalrm *wkalrm)
+{
+	efi_time_t eft;
+	efi_status_t status;
+
+	memset(&eft, 0, sizeof(eft));
+	status = efi.get_wakeup_time((efi_bool_t *)&wkalrm->enabled,
+				     (efi_bool_t *)&wkalrm->pending, &eft);
+	if (status != EFI_SUCCESS)
+		return -EINVAL;
+
+	if (!convert_from_efi_time(&eft, &wkalrm->time))
+		return -EIO;
+
+	return rtc_valid_tm(&wkalrm->time);
+}
+
+static int efi_set_alarm(struct device *dev, struct rtc_wkalrm *wkalrm)
+{
+	efi_time_t eft;
+	efi_status_t status;
+
+	convert_to_efi_time(&wkalrm->time, &eft);
+
+	status = efi.set_wakeup_time((efi_bool_t)!!wkalrm->enabled, &eft);
+
+	return status == EFI_SUCCESS ? 0 : -EINVAL;
+}
+
 static int efi_read_time(struct device *dev, struct rtc_time *tm)
 {
 	efi_status_t status;
@@ -146,13 +176,21 @@ static int efi_set_time(struct device *dev, struct rtc_time *tm)
 
 static int efi_procfs(struct device *dev, struct seq_file *seq)
 {
-	efi_time_t        eft;
+	efi_time_t        eft, alm;
 	efi_time_cap_t    cap;
+	efi_bool_t        enabled, pending;
+	struct rtc_device *rtc = dev_get_drvdata(dev);
 
 	memset(&eft, 0, sizeof(eft));
+	memset(&alm, 0, sizeof(alm));
 	memset(&cap, 0, sizeof(cap));
 
 	efi.get_time(&eft, &cap);
+	if (test_bit(RTC_FEATURE_ALARM, rtc->features) &&
+	    efi.get_wakeup_time(&enabled, &pending, &alm) != EFI_SUCCESS) {
+		enabled = 0;
+		pending = 0;
+	}
 
 	seq_printf(seq,
 		   "Time\t\t: %u:%u:%u.%09u\n"
@@ -168,6 +206,25 @@ static int efi_procfs(struct device *dev, struct seq_file *seq)
 		/* XXX fixme: convert to string? */
 		seq_printf(seq, "Timezone\t: %u\n", eft.timezone);
 
+	if (test_bit(RTC_FEATURE_ALARM, rtc->features)) {
+		seq_printf(seq,
+			   "Alarm Time\t: %u:%u:%u.%09u\n"
+			   "Alarm Date\t: %u-%u-%u\n"
+			   "Alarm Daylight\t: %u\n"
+			   "Enabled\t\t: %s\n"
+			   "Pending\t\t: %s\n",
+			   alm.hour, alm.minute, alm.second, alm.nanosecond,
+			   alm.year, alm.month, alm.day,
+			   alm.daylight,
+			   enabled == 1 ? "yes" : "no",
+			   pending == 1 ? "yes" : "no");
+
+		if (alm.timezone == EFI_UNSPECIFIED_TIMEZONE)
+			seq_puts(seq, "Alarm Timezone\t: unspecified\n");
+		else
+			seq_printf(seq, "Alarm Timezone\t: %d\n", alm.timezone);
+	}
+
 	/*
 	 * now prints the capabilities
 	 */
@@ -183,6 +240,8 @@ static int efi_procfs(struct device *dev, struct seq_file *seq)
 static const struct rtc_class_ops efi_rtc_ops = {
 	.read_time	= efi_read_time,
 	.set_time	= efi_set_time,
+	.read_alarm	= efi_read_alarm,
+	.set_alarm	= efi_set_alarm,
 	.proc		= efi_procfs,
 };
 
@@ -191,6 +250,7 @@ static int __init efi_rtc_probe(struct platform_device *dev)
 	struct rtc_device *rtc;
 	efi_time_t eft;
 	efi_time_cap_t cap;
+	efi_bool_t enabled, pending;
 
 	/* First check if the RTC is usable */
 	if (efi.get_time(&eft, &cap) != EFI_SUCCESS)
@@ -203,7 +263,23 @@ static int __init efi_rtc_probe(struct platform_device *dev)
 	platform_set_drvdata(dev, rtc);
 
 	rtc->ops = &efi_rtc_ops;
-	clear_bit(RTC_FEATURE_ALARM, rtc->features);
+	clear_bit(RTC_FEATURE_UPDATE_INTERRUPT, rtc->features);
+
+	/*
+	 * The EFI_RT_SUPPORTED_WAKEUP_SERVICES bit defaults to enabled
+	 * and only gets cleared when the RT_PROP table explicitly says
+	 * wakeup is unsupported. Many platforms lack an RT_PROP table
+	 * even though they don't implement the wakeup runtime service,
+	 * so probe by actually calling GetWakeupTime() to avoid exposing
+	 * a broken alarm to userspace.
+	 */
+	if (efi_rt_services_supported(EFI_RT_SUPPORTED_WAKEUP_SERVICES) &&
+	    efi.get_wakeup_time(&enabled, &pending, &eft) == EFI_SUCCESS) {
+		set_bit(RTC_F
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clear_bit`
- 外部函数：`device_init_wakeup`
- 外部函数：`get_time`
- 外部函数：`memset`
- 外部函数：`platform_set_drvdata`
- 外部函数：`seq_printf`
- 外部函数：`set_time`
- 大写宏：`EFI_SUCCESS`
- 大写宏：`EINVAL`
- 大写宏：`RTC`
- 大写宏：`RTC_FEATURE_ALARM`
- 大写宏：`XXX`
- 外部类型：`First`
- 外部类型：`Time`
- 外部类型：`Timezone`
- 外部类型：`device`
- 外部类型：`efi_status_t`
- 外部类型：`efi_time_cap_t`
- 外部类型：`efi_time_t`
- 外部类型：`rtc_class_ops`
- 外部类型：`rtc_device`
- 外部类型：`rtc_time`
- 外部类型：`seq_file`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e51cb80687` → 本草稿移入 `cases/defect/auto-linux-e51cb80687/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
