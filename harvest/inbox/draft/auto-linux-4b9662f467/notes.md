# auto-linux-4b9662f467

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4403278e4ea2f654f8130a5c03eb8b4516a0f201](https://github.com/torvalds/linux/commit/4403278e4ea2f654f8130a5c03eb8b4516a0f201) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 67 |
| 编译错误数（gcc syntax-only） | 44（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4403278e4ea2f654f8130a5c03eb8b4516a0f201 (https://github.com/torvalds/linux/commit/4403278e4ea2f654f8130a5c03eb8b4516a0f201)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 491；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4403278e4ea2f654f8130a5c03eb8b4516a0f201 Merge tag 'thermal-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -488,12 +488,12 @@ static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
 	frequency = get_state_freq(cpufreq_cdev, state);
 
 	ret = freq_qos_update_request(&cpufreq_cdev->qos_req, frequency);
-	if (ret >= 0) {
-		cpufreq_cdev->cpufreq_state = state;
-		ret = 0;
-	}
+	if (ret < 0)
+		return ret;
+
+	cpufreq_cdev->cpufreq_state = state;
 
-	return ret;
+	return 0;
 }
 
 /**
@@ -661,8 +661,8 @@ of_cpufreq_cooling_register(struct cpufreq_policy *policy)
 
 		cdev = __cpufreq_cooling_register(np, policy, em);
 		if (IS_ERR(cdev)) {
-			pr_err("cpufreq_cooling: cpu%d failed to register as cooling device: %ld\n",
-			       policy->cpu, PTR_ERR(cdev));
+			pr_err("cpufreq_cooling: cpu%d failed to register as cooling device: %pe\n",
+			       policy->cpu, cdev);
 			cdev = NULL;
 		}
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`arch_scale_cpu_capacity`
- 外部函数：`constraint`
- 外部函数：`cpu_online`
- 外部函数：`cpufreq_quick_get`
- 外部函数：`cpufreq_table_count_valid_entries`
- 外部函数：`cpumask_equal`
- 外部函数：`cpumask_pr_args`
- 外部函数：`cpumask_weight`
- 外部函数：`dev_name`
- 外部函数：`em_is_artificial`
- 外部函数：`em_pd_nr_perf_states`
- 外部函数：`em_perf_state_from_pd`
- 外部函数：`em_span_cpus`
- 外部函数：`free_idle_time`
- 外部函数：`freq_qos_add_request`
- 外部函数：`freq_qos_remove_request`
- 外部函数：`freq_qos_update_request`
- 外部函数：`get_cpu_device`
- 外部函数：`kasprintf`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`levels`
- 外部函数：`pbl`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`pr_warn`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`sched_cpu_util`
- 外部函数：`thermal_of_cooling_device_register`
- 外部函数：`trace_thermal_power_cpu_get_power_simple`
- 外部函数：`trace_thermal_power_cpu_limit`
- 外部函数：`unlikely`
- 大写宏：`CONFIG_SMP`
- 大写宏：`CONFIG_THERMAL_GOV_POWER_ALLOCATOR`
- 大写宏：`CPUFREQ_TABLE_SORTED_ASCENDING`
- 大写宏：`CPUFREQ_TABLE_UNSORTED`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`FREQ_QOS_MAX`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ERR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`MICROWATT_PER_MILLIWATT`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 外部类型：`CPUFreq`
- 外部类型：`Check`
- 外部类型：`Convert`
- 外部类型：`Energy`
- 外部类型：`Failed`
- 外部类型：`Model`
- 外部类型：`No`
- 外部类型：`Otherwise`
- 外部类型：`Request`
- 外部类型：`The`
- 外部类型：`Use`
- 外部类型：`Watts`
- 外部类型：`cpufreq_policy`
- 外部类型：`device`
- 外部类型：`device_node`
- 外部类型：`em_perf_domain`
- 外部类型：`em_perf_state`
- 外部类型：`freq_qos_request`
- 外部类型：`thermal_cooling_device`
- 外部类型：`thermal_cooling_device_ops`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4b9662f467` → 本草稿移入 `cases/defect/auto-linux-4b9662f467/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
