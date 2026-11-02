# auto-linux-69896d7d29

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9](https://github.com/torvalds/linux/commit/7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9 (https://github.com/torvalds/linux/commit/7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9 Merge tag 'timers-urgent-2026-11-01' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -12,6 +12,7 @@
 #ifndef _LINUX_HRTIMER_H
 #define _LINUX_HRTIMER_H
 
+#include <linux/cleanup.h>
 #include <linux/hrtimer_rearm.h>
 #include <linux/hrtimer_types.h>
 #include <linux/init.h>
@@ -201,6 +202,29 @@ extern void destroy_hrtimer_on_stack(struct hrtimer *timer);
 static inline void destroy_hrtimer_on_stack(struct hrtimer *timer) { }
 #endif
 
+/*
+ * The cleanup class pairs on-stack setup with debug-object teardown. Callers
+ * using it must not call destroy_hrtimer_on_stack() manually.
+ *
+ * This class provides debug-object teardown, not timer cancellation. Callers
+ * must ensure that no hrtimer operation can still access the timer before
+ * leaving the scope. In particular, the timer must no longer be queued and
+ * any running callback must have completed.
+ *
+ * Callers using hrtimer_setup_on_stack() without this class retain
+ * responsibility for destroying the timer before its backing storage leaves
+ * scope.
+ */
+DEFINE_CLASS(hrtimer_on_stack, struct hrtimer *,
+	     destroy_hrtimer_on_stack(_T),
+	     ({
+		hrtimer_setup_on_stack(timer, function, clock_id, mode);
+		timer;
+	     }),
+	     struct hrtimer *timer,
+	     enum hrtimer_restart (*function)(struct hrtimer *),
+	     clockid_t clock_id, enum hrtimer_mode mode)
+
 /* Basic timer operations: */
 extern void hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
 				   u64 range_ns, const enum hrtimer_mode mode);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`hrtimer_start_range_ns`
- 大写宏：`HRTIMER_MODE_ABS`
- 大写宏：`HRTIMER_MODE_ABS_HARD`
- 大写宏：`HRTIMER_MODE_ABS_PINNED`
- 大写宏：`HRTIMER_MODE_ABS_PINNED_HARD`
- 大写宏：`HRTIMER_MODE_ABS_PINNED_SOFT`
- 大写宏：`HRTIMER_MODE_ABS_SOFT`
- 大写宏：`HRTIMER_MODE_HARD`
- 大写宏：`HRTIMER_MODE_LAZY_REARM`
- 大写宏：`HRTIMER_MODE_PINNED`
- 大写宏：`HRTIMER_MODE_REL`
- 大写宏：`HRTIMER_MODE_REL_HARD`
- 大写宏：`HRTIMER_MODE_REL_PINNED`
- 大写宏：`HRTIMER_MODE_REL_PINNED_HARD`
- 大写宏：`HRTIMER_MODE_REL_PINNED_SOFT`
- 大写宏：`HRTIMER_MODE_REL_SOFT`
- 大写宏：`HRTIMER_MODE_SOFT`
- 外部类型：`Basic`
- 外部类型：`hrtimer`
- 外部类型：`ktime_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-69896d7d29` → 本草稿移入 `cases/defect/auto-linux-69896d7d29/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
