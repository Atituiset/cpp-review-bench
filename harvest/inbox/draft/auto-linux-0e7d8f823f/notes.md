# auto-linux-0e7d8f823f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b1fa457bddf009907b023bc0c09ba4eca3191a8e](https://github.com/torvalds/linux/commit/b1fa457bddf009907b023bc0c09ba4eca3191a8e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b1fa457bddf009907b023bc0c09ba4eca3191a8e (https://github.com/torvalds/linux/commit/b1fa457bddf009907b023bc0c09ba4eca3191a8e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1594；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b1fa457bddf009907b023bc0c09ba4eca3191a8e Merge tag 'cgroup-for-7.3-rc4-fixes-2' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/cgroup :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1591,10 +1591,11 @@ static int remote_partition_enable(struct cpuset *cs, int new_prs,
 	 * above it or remote partition root underneath it is not allowed.
 	 */
 	compute_excpus(cs, tmp->new_cpus);
-	WARN_ON_ONCE(cpumask_intersects(tmp->new_cpus, subpartitions_cpus));
 	if (!cpumask_intersects(tmp->new_cpus, cpu_active_mask) ||
 	    cpumask_subset(top_cpuset.effective_cpus, tmp->new_cpus))
 		return PERR_INVCPUS;
+	if (cpumask_intersects(tmp->new_cpus, subpartitions_cpus))
+		return PERR_NOCPUS;
 	if (((new_prs == PRS_ISOLATED) &&
 	     !isolated_cpus_can_update(tmp->new_cpus, NULL)) ||
 	    prstate_housekeeping_conflict(new_prs, tmp->new_cpus))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`alloc_cpumask_var`
- 外部函数：`cpumask_and`
- 外部函数：`cpumask_andnot`
- 外部函数：`cpumask_empty`
- 外部函数：`cpumask_intersects`
- 外部函数：`cpumask_subset`
- 外部函数：`cpumask_weight_and`
- 外部函数：`cpumask_weight_andnot`
- 外部函数：`free_cpumask_var`
- 外部函数：`housekeeping_cpumask`
- 外部函数：`housekeeping_enabled`
- 外部函数：`parent_cs`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HK_TYPE_DOMAIN`
- 大写宏：`HK_TYPE_DOMAIN_BOOT`
- 大写宏：`HK_TYPE_KERNEL_NOISE`
- 大写宏：`NULL`
- 大写宏：`PERR_INVCPUS`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`CPUs`
- 外部类型：`If`
- 外部类型：`Otherwise`
- 外部类型：`Remove`
- 外部类型：`cgroup_subsys_state`
- 外部类型：`cpumask`
- 外部类型：`cpumask_var_t`
- 外部类型：`cpuset`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0e7d8f823f` → 本草稿移入 `cases/defect/auto-linux-0e7d8f823f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
