# auto-linux-f3f366a493

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
| 外部依赖数（dep_count） | 56 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #100638f0f016cb0e6c75e95ff2b0495bca0b3054 (https://github.com/torvalds/linux/commit/100638f0f016cb0e6c75e95ff2b0495bca0b3054)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 100638f0f016cb0e6c75e95ff2b0495bca0b3054 Merge tag 'pm-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -420,6 +420,9 @@ struct cpufreq_driver {
 	/* Will be called after the driver is fully initialized */
 	void		(*ready)(struct cpufreq_policy *policy);
 
+	/* Return the capacity reference frequency for policy. */
+	unsigned int	(*scale_freq_ref)(struct cpufreq_policy *policy);
+
 	struct freq_attr **attr;
 
 	/* platform specific boost support code */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`altogether`
- 外部函数：`cpufreq_enable_fast_switch`
- 外部函数：`driver`
- 外部函数：`flag`
- 外部函数：`int`
- 外部函数：`ssize_t`
- 外部函数：`update_policy`
- 外部函数：`void`
- 大写宏：`ACPI`
- 大写宏：`ALL`
- 大写宏：`ANY`
- 大写宏：`CPU`
- 大写宏：`CPUFREQ_GOV_STRICT_TARGET`
- 大写宏：`CPUFREQ_RELATION_E`
- 大写宏：`CPUFREQ_TABLE_SORTED_ASCENDING`
- 大写宏：`CPUFREQ_TABLE_SORTED_DESCENDING`
- 大写宏：`CPUFREQ_TABLE_UNSORTED`
- 大写宏：`DVFS`
- 大写宏：`IRQ`
- 外部类型：`Any`
- 外部类型：`CPUs`
- 外部类型：`Cached`
- 外部类型：`Fast`
- 外部类型：`For`
- 外部类型：`Not`
- 外部类型：`Offline`
- 外部类型：`Online`
- 外部类型：`Pending`
- 外部类型：`Per`
- 外部类型：`Pointer`
- 外部类型：`Preferred`
- 外部类型：`Related`
- 外部类型：`Remote`
- 外部类型：`Set`
- 外部类型：`Should`
- 外部类型：`Synchronization`
- 外部类型：`Task`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Tracks`
- 外部类型：`Will`
- 外部类型：`attribute`
- 外部类型：`clk`
- 外部类型：`completion`
- 外部类型：`cpufreq_stats`
- 外部类型：`cpumask_var_t`
- 外部类型：`freq_constraints`
- 外部类型：`freq_qos_request`
- 外部类型：`kobject`
- 外部类型：`list_head`
- 外部类型：`module`
- 外部类型：`notifier_block`
- 外部类型：`rw_semaphore`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`task_struct`
- 外部类型：`thermal_cooling_device`
- 外部类型：`wait_queue_head_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-f3f366a493` → 本草稿移入 `cases/defect/auto-linux-f3f366a493/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
