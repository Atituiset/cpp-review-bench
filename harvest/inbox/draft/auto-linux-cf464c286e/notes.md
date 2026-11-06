# auto-linux-cf464c286e

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
| 外部依赖数（dep_count） | 64 |
| 编译错误数（gcc syntax-only） | 33（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3e516eae690b3f24c19168cc0fe605e1c610eb21 (https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 410；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3e516eae690b3f24c19168cc0fe605e1c610eb21 Merge tag 'hwmon-for-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -76,7 +76,7 @@ struct temp_data {
 	int temp;
 	bool temp_valid;
 	int tjmax;
-	unsigned long last_updated;
+	u64 last_updated;
 	unsigned int cpu;
 	int index;
 	u32 cpu_core_id;
@@ -407,7 +407,7 @@ static ssize_t show_temp(struct device *dev,
 	mutex_lock(&tdata->update_lock);
 
 	/* Check whether the time interval has elapsed */
-	if (time_after(jiffies, tdata->last_updated + HZ)) {
+	if (time_after64(get_jiffies_64(), tdata->last_updated + HZ)) {
 		tjmax = get_tjmax(tdata, dev);
 		rdmsrq_on_cpu(tdata->cpu, tdata->status_reg, &val.q);
 		/*
@@ -417,7 +417,7 @@ static ssize_t show_temp(struct device *dev,
 		 */
 		tdata->temp_valid = (val.l >> 31) & 1;
 		tdata->temp = tjmax - ((val.l >> 16) & 0xff) * 1000;
-		tdata->last_updated = jiffies;
+		tdata->last_updated = get_jiffies_64();
 	}
 
 	valid = tdata->temp_valid;
@@ -491,11 +491,12 @@ init_temp_data(struct platform_data *pdata, unsigned int cpu, int pkg_flag)
 	struct temp_data *tdata;
 
 	if (!pdata->core_data) {
-		pdata->nr_cores = topology_num_cores_per_package();
-		pdata->core_data = kzalloc_objs(struct temp_data *,
-						pdata->nr_cores);
+		int nr_cores = topology_num_cores_per_package();
+
+		pdata->core_data = kzalloc_objs(struct temp_data *, nr_cores);
 		if (!pdata->core_data)
 			return NULL;
+		pdata->nr_cores = nr_cores;
 	}
 
 	tdata = kzalloc_obj(struct temp_data);
@@ -520,11 +521,8 @@ init_temp_data(struct platform_data *pdata, unsigned int cpu, int pkg_flag)
 	tdata->cpu = cpu;
 	tdata->cpu_core_id = topology_core_id(cpu);
 	tdata->attr_size = MAX_CORE_ATTRS;
-	/*
-	 * A zero timestamp does not look stale on 32-bit, where jiffies
-	 * starts just short of wrapping. Backdate it instead.
-	 */
-	tdata->last_updated = jiffies - HZ - 1;
+	/* Backdate to force a refresh on the first read */
+	tdata->last_updated = get_jiffies_64() - HZ - 1;
 	mutex_init(&tdata->update_lock);
 	return tdata;
 }
@@ -695,6 +693,9 @@ static int coretemp_cpu_online(unsigned int cpu)
 	if (!housekeeping_cpu(cpu, HK_TYPE_MISC))
 		return 0;
 
+	if (!pdev)
+		return 0;
+
 	pdata = platform_get_drvdata(pdev);
 	if (!pdata->hwmon_dev) {
 		struct device *hwmon;
@@ -746,6 +747,9 @@ static int coretemp_cpu_offline(unsigned int cpu)
 		return 0;
 
 	/* If the physical CPU device does not exist, just return */
+	if (!pdev)
+		return 0;
+
 	pd = platform_get_drvdata(pdev);
 	if (!pd->hwmon_dev)
 		return 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpu_data`
- 外部函数：`dev_notice`
- 外部函数：`dev_warn`
- 外部函数：`dev_warn_once`
- 外部函数：`housekeeping_cpu`
- 外部函数：`kzalloc_obj`
- 外部函数：`kzalloc_objs`
- 外部函数：`mutex_init`
- 外部函数：`mutex_lock`
- 外部函数：`pci_dev_put`
- 外部函数：`pci_get_domain_bus_and_slot`
- 外部函数：`platform_get_drvdata`
- 外部函数：`rdmsrq_on_cpu`
- 外部函数：`rdmsrq_safe_on_cpu`
- 外部函数：`strstr`
- 外部函数：`time_after`
- 外部函数：`topology_core_id`
- 外部函数：`topology_num_cores_per_package`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`CPU`
- 大写宏：`HK_TYPE_MISC`
- 大写宏：`IA32_TEMPERATURE_TARGET`
- 大写宏：`INTEL_CORE2_MEROM`
- 大写宏：`INTEL_CORE2_PENRYN`
- 大写宏：`INTEL_CORE_YONAH`
- 大写宏：`MAX_CORE_ATTRS`
- 大写宏：`MSR`
- 大写宏：`MSR_IA32_TEMPERATURE_TARGET`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`PCI`
- 大写宏：`PCI_DEVFN`
- 大写宏：`PCI_VENDOR_ID_INTEL`
- 大写宏：`XXX`
- 外部类型：`An`
- 外部类型：`Atom`
- 外部类型：`Backdate`
- 外部类型：`CPUs`
- 外部类型：`Check`
- 外部类型：`Core2`
- 外部类型：`Early`
- 外部类型：`Explicit`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`IDs`
- 外部类型：`If`
- 外部类型：`Intel`
- 外部类型：`Just`
- 外部类型：`Mobile`
- 外部类型：`Mobility`
- 外部类型：`Not`
- 外部类型：`Now`
- 外部类型：`Penryn`
- 外部类型：`Platform`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TjMax`
- 外部类型：`Tjmax`
- 外部类型：`Trust`
- 外部类型：`Unable`
- 外部类型：`Using`
- 外部类型：`Wiki`
- 外部类型：`cpuinfo_x86`
- 外部类型：`device`
- 外部类型：`pci_dev`
- 外部类型：`temp_data`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-cf464c286e` → 本草稿移入 `cases/defect/auto-linux-cf464c286e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
