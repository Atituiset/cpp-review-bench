# auto-linux-57e3e680eb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#df37e3924292fb41d325721008520ba93deced42](https://github.com/torvalds/linux/commit/df37e3924292fb41d325721008520ba93deced42) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 48 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #df37e3924292fb41d325721008520ba93deced42 (https://github.com/torvalds/linux/commit/df37e3924292fb41d325721008520ba93deced42)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 102；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT df37e3924292fb41d325721008520ba93deced42 Merge tag 'pm-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5,11 +5,13 @@
  */
 
 #include <linux/bitfield.h>
+#include <linux/limits.h>
 
 #include <acpi/cppc_acpi.h>
 #include <asm/msr.h>
 #include <asm/processor.h>
 #include <asm/topology.h>
+#include <asm/cpu_device_id.h>
 
 #define CPPC_HIGHEST_PERF_PERFORMANCE	196
 #define CPPC_HIGHEST_PERF_PREFCORE	166
@@ -45,10 +47,20 @@ bool cpc_ffh_supported(void)
 	return true;
 }
 
+static bool cpc_ffh_reg_valid(const struct cpc_reg *reg)
+{
+	return reg->address <= U32_MAX && reg->bit_width &&
+	       reg->bit_width <= 64 &&
+	       reg->bit_offset <= 64 - reg->bit_width;
+}
+
 int cpc_read_ffh(int cpunum, struct cpc_reg *reg, u64 *val)
 {
 	int err;
 
+	if (!cpc_ffh_reg_valid(reg))
+		return -EINVAL;
+
 	err = rdmsrq_safe_on_cpu(cpunum, reg->address, val);
 	if (!err) {
 		u64 mask = GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
@@ -65,6 +77,9 @@ int cpc_write_ffh(int cpunum, struct cpc_reg *reg, u64 val)
 	u64 rd_val;
 	int err;
 
+	if (!cpc_ffh_reg_valid(reg))
+		return -EINVAL;
+
 	err = rdmsrq_safe_on_cpu(cpunum, reg->address, &rd_val);
 	if (!err) {
 		u64 mask = GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
@@ -81,31 +96,18 @@ int cpc_write_ffh(int cpunum, struct cpc_reg *reg, u64 val)
 
 static void amd_set_max_freq_ratio(void)
 {
-	struct cppc_perf_caps perf_caps;
-	u64 numerator, nominal_perf;
+	u64 numerator, denominator;
 	u64 perf_ratio;
 	int rc;
 
-	rc = cppc_get_perf_caps(0, &perf_caps);
+	rc = amd_get_boost_ratio(0, &numerator, &denominator);
 	if (rc) {
-		pr_debug("Could not retrieve perf counters (%d)\n", rc);
-		return;
-	}
-
-	rc = amd_get_boost_ratio_numerator(0, &numerator);
-	if (rc) {
-		pr_debug("Could not retrieve highest performance (%d)\n", rc);
-		return;
-	}
-	nominal_perf = perf_caps.nominal_perf;
-
-	if (!nominal_perf) {
-		pr_debug("Could not retrieve nominal performance\n");
+		pr_debug("Could not retrieve boost ratio (%d)\n", rc);
 		return;
 	}
 
 	/* midpoint between max_boost and max_P */
-	perf_ratio = (div_u64(numerator * SCHED_CAPACITY_SCALE, nominal_perf) + SCHED_CAPACITY_SCALE) >> 1;
+	perf_ratio = (div_u64(numerator * SCHED_CAPACITY_SCALE, denominator) + SCHED_CAPACITY_SCALE) >> 1;
 
 	freq_invariance_set_perf_ratio(perf_ratio, false);
 }
@@ -225,35 +227,30 @@ int amd_detect_prefcore(bool *detected)
 EXPORT_SYMBOL_GPL(amd_detect_prefcore);
 
 /**
- * amd_get_boost_ratio_numerator: Get the numerator to use for boost ratio calculation
- * @cpu: CPU to get numerator for.
- * @numerator: Output variable for numerator.
- *
- * Determine the numerator to use for calculating the boost ratio on
- * a CPU. On systems that support preferred cores, this will be a hardcoded
- * value. On other systems this will the highest performance register value.
+ * amd_get_effective_highest_perf: Get the effective highest performance value
+ * @cpu: CPU to get highest performance for.
  *
- * If booting the system with amd-pstate enabled but preferred cores disabled then
- * the correct boost numerator will be returned to match hardware capabilities
- * even if the preferred cores scheduling hints are not enabled.
+ * Get the effective highest performance value for a CPU, accounting for
+ * preferred cores and heterogeneous topologies. On systems with preferred
+ * cores, this may be a hardcoded value. On heterogeneous systems, this
+ * may be a per-CPU value. On other systems, this is the shared highest
+ * performance value.
  *
- * Return: 0 for success, negative error code otherwise.
+ * Return: Effective highest performance value, or negative error code.
  */
-int amd_get_boost_ratio_numerator(unsigned int cpu, u64 *numerator)
+int amd_get_effective_highest_perf(unsigned int cpu)
 {
 	bool prefcore;
 	int ret;
 	u32 tmp;
 
 	ret = amd_detect_prefcore(&prefcore);
-	if (ret)
+	if (ret < 0)
 		return ret;
 
 	/* without preferred cores, return the highest perf register value */
-	if (!prefcore) {
-		*numerator = boost_numerator;
-		return 0;
-	}
+	if (!prefcore)
+		return boost_numerator;
 
 	/*
 	 * 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`counters`
- 外部函数：`cppc_get_highest_perf`
- 外部函数：`cppc_get_perf_caps`
- 外部函数：`cpu_data`
- 外部函数：`cpu_feature_enabled`
- 外部函数：`div_u64`
- 外部函数：`freq_invariance_set_perf_ratio`
- 外部函数：`performance`
- 外部函数：`pr_debug`
- 外部函数：`pr_warn`
- 外部函数：`rdmsrq_safe_on_cpu`
- 外部函数：`ssupported`
- 大写宏：`AMD`
- 大写宏：`AMD_CPPC_HIGHEST_PERF_MASK`
- 大写宏：`AMD_PREF_CORE_SUPPORTED`
- 大写宏：`AMD_PREF_CORE_UNSUPPORTED`
- 大写宏：`CPPC`
- 大写宏：`CPU`
- 大写宏：`EINVAL`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`FIELD_GET`
- 大写宏：`GENMASK_ULL`
- 大写宏：`MSR_AMD_CPPC_CAP1`
- 大写宏：`SCHED_CAPACITY_SCALE`
- 大写宏：`TOPO_CPU_TYPE_ANY`
- 大写宏：`TOPO_CPU_TYPE_EFFICIENCY`
- 大写宏：`TOPO_CPU_TYPE_LOW_POWER`
- 大写宏：`TOPO_CPU_TYPE_PERFORMANCE`
- 大写宏：`TOPO_CPU_TYPE_UNKNOWN`
- 大写宏：`WARN_ON`
- 大写宏：`WRITE_ONCE`
- 大写宏：`X86_FEATURE_AMD_HTR_CORES`
- 大写宏：`X86_FEATURE_CPPC`
- 大写宏：`X86_FEATURE_ZEN4`
- 外部类型：`CPUs`
- 外部类型：`Could`
- 外部类型：`Determine`
- 外部类型：`Family`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Model`
- 外部类型：`On`
- 外部类型：`Output`
- 外部类型：`Return`
- 外部类型：`Undefined`
- 外部类型：`cpc_reg`
- 外部类型：`cppc_perf_caps`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-57e3e680eb` → 本草稿移入 `cases/defect/auto-linux-57e3e680eb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
