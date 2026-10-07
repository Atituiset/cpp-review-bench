# auto-linux-8d663e4b28

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b8eb5fd5bdb4c03758b056c70e5e1d962ca89757](https://github.com/torvalds/linux/commit/b8eb5fd5bdb4c03758b056c70e5e1d962ca89757) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b8eb5fd5bdb4c03758b056c70e5e1d962ca89757 (https://github.com/torvalds/linux/commit/b8eb5fd5bdb4c03758b056c70e5e1d962ca89757)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4065；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b8eb5fd5bdb4c03758b056c70e5e1d962ca89757 cgroup/cpuset: Call is_in_v2_mode() after acquiring cpuset_mutex in cpuset_handle_hotplug() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4062,15 +4062,16 @@ static void cpuset_handle_hotplug(void)
 	static cpumask_t new_cpus;
 	static nodemask_t new_mems;
 	bool cpus_updated, mems_updated;
-	bool on_dfl = is_in_v2_mode();
+	bool on_dfl;
 	struct tmpmasks tmp, *ptmp = NULL;
 
-	if (on_dfl && !alloc_tmpmasks(&tmp))
-		ptmp = &tmp;
-
 	lockdep_assert_cpus_held();
 	mutex_lock(&cpuset_mutex);
 
+	on_dfl = is_in_v2_mode();
+	if (on_dfl && !alloc_tmpmasks(&tmp))
+		ptmp = &tmp;
+
 	/* fetch the available cpus/mems and find out which changed how */
 	cpumask_copy(&new_cpus, cpu_active_mask);
 	new_mems = node_states[N_MEMORY];
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cgroup_subsys_on_dfl`
- 外部函数：`cpumask_copy`
- 外部函数：`free_cpumask_var`
- 外部函数：`lockdep_assert_cpus_held`
- 外部函数：`masks`
- 外部函数：`mutex_lock`
- 外部函数：`update_cpumasks_hier`
- 外部函数：`zalloc_cpumask_var`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`CONFIG_CPUSETS_V1`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ENABLED`
- 大写宏：`NULL`
- 大写宏：`N_MEMORY`
- 大写宏：`READ_ONCE`
- 外部类型：`Array`
- 外部类型：`For`
- 外部类型：`Note`
- 外部类型：`cpumask_t`
- 外部类型：`cpumask_var_t`
- 外部类型：`nodemask_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8d663e4b28` → 本草稿移入 `cases/defect/auto-linux-8d663e4b28/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
