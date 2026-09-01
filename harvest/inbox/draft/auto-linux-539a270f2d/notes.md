# auto-linux-539a270f2d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c3b510de420d70def08190083d388e0873c1aa84](https://github.com/torvalds/linux/commit/c3b510de420d70def08190083d388e0873c1aa84) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c3b510de420d70def08190083d388e0873c1aa84 (https://github.com/torvalds/linux/commit/c3b510de420d70def08190083d388e0873c1aa84)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1273；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c3b510de420d70def08190083d388e0873c1aa84 Merge tag 'cgroup-for-7.3-rc1-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/cgroup :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1259,6 +1259,28 @@ static void reset_partition_data(struct cpuset *cs)
 		cpumask_copy(cs->effective_cpus, parent->effective_cpus);
 }
 
+/* Return true if isolated_cpus changes. */
+static bool isolated_cpu_update(int new_prs, int cpu)
+{
+	lockdep_assert_held(&callback_lock);
+	lockdep_assert_held(&cpuset_mutex);
+
+	if (new_prs == PRS_ISOLATED) {
+		if (cpumask_test_cpu(cpu, isolated_cpus))
+			return false;
+		cpumask_set_cpu(cpu, isolated_cpus);
+		return true;
+	}
+
+	/* CPUs isolated at boot must remain isolated. */
+	if (!cpumask_test_cpu(cpu,
+			      housekeeping_cpumask(HK_TYPE_DOMAIN_BOOT)) ||
+	    !cpumask_test_cpu(cpu, isolated_cpus))
+		return false;
+	cpumask_clear_cpu(cpu, isolated_cpus);
+	return true;
+}
+
 /*
  * isolated_cpus_update - Update the isolated_cpus mask
  * @old_prs: old partition_root_state
@@ -1267,19 +1289,16 @@ static void reset_partition_data(struct cpuset *cs)
  */
 static void isolated_cpus_update(int old_prs, int new_prs, struct cpumask *xcpus)
 {
+	bool updated = false;
+	int cpu;
+
 	WARN_ON_ONCE(old_prs == new_prs);
 	lockdep_assert_held(&callback_lock);
 	lockdep_assert_held(&cpuset_mutex);
-	if (new_prs == PRS_ISOLATED) {
-		if (cpumask_subset(xcpus, isolated_cpus))
-			return;
-		cpumask_or(isolated_cpus, isolated_cpus, xcpus);
-	} else {
-		if (!cpumask_intersects(xcpus, isolated_cpus))
-			return;
-		cpumask_andnot(isolated_cpus, isolated_cpus, xcpus);
-	}
-	update_housekeeping = true;
+	for_each_cpu(cpu, xcpus)
+		updated |= isolated_cpu_update(new_prs, cpu);
+	if (updated)
+		update_housekeeping = true;
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpumask_andnot`
- 外部函数：`cpumask_copy`
- 外部函数：`cpumask_intersects`
- 外部函数：`cpumask_or`
- 外部函数：`cpumask_subset`
- 外部函数：`lockdep_assert_held`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Update`
- 外部类型：`cpumask`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-539a270f2d` → 本草稿移入 `cases/defect/auto-linux-539a270f2d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
