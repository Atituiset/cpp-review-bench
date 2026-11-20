# auto-linux-6b71f6d8a0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#d40cb8a12ed25789f39404c5f7968a65e98d89c3](https://github.com/torvalds/linux/commit/d40cb8a12ed25789f39404c5f7968a65e98d89c3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 58 |
| 编译错误数（gcc syntax-only） | 145（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d40cb8a12ed25789f39404c5f7968a65e98d89c3 (https://github.com/torvalds/linux/commit/d40cb8a12ed25789f39404c5f7968a65e98d89c3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d40cb8a12ed25789f39404c5f7968a65e98d89c3 Merge branches 'acpi-cppc-fixes' and 'pm-cpufreq-fixes' :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2452,6 +2452,14 @@ int cpufreq_driver_target(struct cpufreq_policy *policy,
 {
 	guard(cpufreq_policy_write)(policy);
 
+	/*
+	 * The last CPU in the policy may have gone offline after the caller
+	 * obtained it, in which case the policy is inactive and must not be
+	 * used for changing the frequency.
+	 */
+	if (unlikely(policy_is_inactive(policy)))
+		return -EBUSY;
+
 	return __cpufreq_driver_target(policy, target_freq, relation);
 }
 EXPORT_SYMBOL_GPL(cpufreq_driver_target);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_begin`
- 外部函数：`arch_scale_freq_ref`
- 外部函数：`arch_set_freq_scale`
- 外部函数：`clamp_val`
- 外部函数：`cpufreq_frequency_table_target`
- 外部函数：`cpufreq_scale`
- 外部函数：`cpufreq_stats_record_transition`
- 外部函数：`cpumask_pr_args`
- 外部函数：`get_intermediate`
- 外部函数：`irqs_disabled`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`srcu_notifier_call_chain`
- 外部函数：`swap`
- 外部函数：`sysfs_emit_at`
- 外部函数：`target_intermediate`
- 外部函数：`trace_cpu_frequency`
- 外部函数：`unlikely`
- 外部函数：`wait_event`
- 外部函数：`wake_up`
- 大写宏：`ASYNC_NOTIFICATION`
- 大写宏：`BUG_ON`
- 大写宏：`CONFIG_SMP`
- 大写宏：`CPU`
- 大写宏：`CPUFREQ_ASYNC_NOTIFICATION`
- 大写宏：`CPUFREQ_CONST_LOOPS`
- 大写宏：`CPUFREQ_POSTCHANGE`
- 大写宏：`CPUFREQ_PRECHANGE`
- 大写宏：`EINVAL`
- 大写宏：`FREQ`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`WARN_ON`
- 外部类型：`CPUs`
- 外部类型：`Catch`
- 外部类型：`Detect`
- 外部类型：`Failed`
- 外部类型：`Moreover`
- 外部类型：`Save`
- 外部类型：`Warning`
- 外部类型：`We`
- 外部类型：`cpufreq_freqs`
- 外部类型：`cpufreq_policy`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-6b71f6d8a0` → 本草稿移入 `cases/defect/auto-linux-6b71f6d8a0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
