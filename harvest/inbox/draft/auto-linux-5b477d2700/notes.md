# auto-linux-5b477d2700

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#88328f850d956d267d11e2ee225ee1eaa902b8e0](https://github.com/torvalds/linux/commit/88328f850d956d267d11e2ee225ee1eaa902b8e0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #88328f850d956d267d11e2ee225ee1eaa902b8e0 (https://github.com/torvalds/linux/commit/88328f850d956d267d11e2ee225ee1eaa902b8e0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 13（原始 PR diff 行 6467；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 88328f850d956d267d11e2ee225ee1eaa902b8e0 Merge tag 'sched-urgent-2026-10-18' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6463,12 +6463,20 @@ place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
 		load = cfs_rq->sum_weight;
 		if (curr && curr->on_rq)
 			load += avg_vruntime_weight(cfs_rq, curr->h_load.weight);
-
-		weight = avg_vruntime_weight(cfs_rq, se->h_load.weight);
-		lag *= load + weight;
 		if (WARN_ON_ONCE(!load))
 			load = 1;
-		lag = div64_long(lag, load);
+
+		weight = avg_vruntime_weight(cfs_rq, se->h_load.weight);
+
+		/*
+		 * Rewrite, because the product (W + w_i)*vl'_i might
+		 * overflow s64:
+		 *
+		 *   vl_i = (W + w_i)*vl'_i / W
+		 *        = (W*vl'_i + w_i*vl'_i) / W
+		 *        = vl'_i + w_i*vl'_i/W
+		 */
+		lag += div64_long(lag * weight, load);
 
 		/*
 		 * A heavy entity (relative to the tree) will pull the
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`div64_long`
- 外部函数：`entity`
- 外部函数：`max`
- 大写宏：`CONFIG_64BIT`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`cfs_rq`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-5b477d2700` → 本草稿移入 `cases/defect/auto-linux-5b477d2700/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
