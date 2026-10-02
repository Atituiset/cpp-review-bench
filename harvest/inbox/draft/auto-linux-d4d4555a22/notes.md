# auto-linux-d4d4555a22

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#100638f0f016cb0e6c75e95ff2b0495bca0b3054](https://github.com/torvalds/linux/commit/100638f0f016cb0e6c75e95ff2b0495bca0b3054) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 140 |
| 编译错误数（gcc syntax-only） | 165（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #100638f0f016cb0e6c75e95ff2b0495bca0b3054 (https://github.com/torvalds/linux/commit/100638f0f016cb0e6c75e95ff2b0495bca0b3054)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 100638f0f016cb0e6c75e95ff2b0495bca0b3054 Merge tag 'pm-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1135,6 +1135,14 @@ static bool hybrid_clear_max_perf_cpu(void)
 	return ret;
 }
 
+static unsigned int intel_pstate_scale_freq_ref(struct cpufreq_policy *policy)
+{
+	if (READ_ONCE(all_cpu_data[policy->cpu]->capacity_perf))
+		return policy->cpuinfo.max_freq;
+
+	return 0;
+}
+
 static void intel_pstate_update_freq_limits(struct cpudata *cpu)
 {
 	int scaling = cpu->pstate.scaling;
@@ -3088,6 +3096,7 @@ static struct cpufreq_driver intel_pstate = {
 	.offline	= intel_pstate_cpu_offline,
 	.online		= intel_pstate_cpu_online,
 	.update_limits	= intel_pstate_update_limits,
+	.scale_freq_ref = intel_pstate_scale_freq_ref,
 	.name		= "intel_pstate",
 };
 
@@ -3411,6 +3420,7 @@ static struct cpufreq_driver intel_cpufreq = {
 	.suspend	= intel_cpufreq_suspend,
 	.resume		= intel_pstate_resume,
 	.update_limits	= intel_pstate_update_limits,
+	.scale_freq_ref = intel_pstate_scale_freq_ref,
 	.name		= "intel_cpufreq",
 };
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acpi_processor_register_performance`
- 外部函数：`acpi_processor_unregister_performance`
- 外部函数：`boot_cpu_has`
- 外部函数：`cppc_get_perf_caps`
- 外部函数：`int`
- 外部函数：`intel_pstate_hwp_boost_down`
- 外部函数：`intel_pstate_hwp_boost_up`
- 外部函数：`pr_debug`
- 外部函数：`rdmsrq_on_cpu`
- 外部函数：`sched_set_itmt_core_prio`
- 外部函数：`sched_set_itmt_support`
- 外部函数：`schedule_work`
- 外部函数：`u64`
- 外部函数：`void`
- 大写宏：`ACPI`
- 大写宏：`ACPI_ADR_SPACE_FIXED_HARDWARE`
- 大写宏：`CONFIG_ACPI`
- 大写宏：`CONFIG_ENERGY_MODEL`
- 大写宏：`CPPC`
- 大写宏：`CPU`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EPP`
- 大写宏：`GENMASK_ULL`
- 大写宏：`HWP`
- 大写宏：`HWP_HIGHEST_PERF`
- 大写宏：`ITMT`
- 大写宏：`MSR`
- 大写宏：`MSR_HWP_CAPABILITIES`
- 大写宏：`MSR_HWP_REQUEST`
- 大写宏：`PERF_CTL`
- 大写宏：`PM_ENTERPRISE_SERVER`
- 大写宏：`PM_PERFORMANCE_SERVER`
- 大写宏：`READ_ONCE`
- 大写宏：`U32_MAX`
- 大写宏：`U8_MAX`
- 大写宏：`X86_FEATURE_HWP_EPP`
- 外部类型：`Also`
- 外部类型：`Check`
- 外部类型：`Fall`
- 外部类型：`If`
- 外部类型：`MHz`
- 外部类型：`Queue`
- 外部类型：`Request`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Use`
- 外部类型：`When`
- 外部类型：`acpi_processor_performance`
- 外部类型：`cppc_perf_caps`
- 外部类型：`cpufreq_policy`
- 外部类型：`delayed_work`
- 外部类型：`int32_t`
- 外部类型：`sample`
- 外部类型：`update_util_data`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d4d4555a22` → 本草稿移入 `cases/defect/auto-linux-d4d4555a22/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
