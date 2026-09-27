# auto-linux-f80d2efac7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#efb27d47677397961c9017c0f8f469eb25a15d68](https://github.com/torvalds/linux/commit/efb27d47677397961c9017c0f8f469eb25a15d68) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #efb27d47677397961c9017c0f8f469eb25a15d68 (https://github.com/torvalds/linux/commit/efb27d47677397961c9017c0f8f469eb25a15d68)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 529；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT efb27d47677397961c9017c0f8f469eb25a15d68 Merge tag 'probes-fixes-v7.3-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -42,6 +42,7 @@
 #include <linux/execmem.h>
 #include <linux/cleanup.h>
 #include <linux/wait.h>
+#include <linux/wait_bit.h>
 
 #include <asm/sections.h>
 #include <asm/cacheflush.h>
@@ -526,7 +527,8 @@ enum {
 	OPTIMIZER_ST_FLUSHING = 2,
 };
 
-static DECLARE_COMPLETION(optimizer_completion);
+/* Bumped at the end of each kprobe_optimizer() pass, under 'kprobe_mutex' */
+static unsigned long optimizer_passes;
 
 #define OPTIMIZE_DELAY 5
 
@@ -654,9 +656,9 @@ static void kprobe_optimizer(void)
 		do_free_cleaned_kprobes();
 	}
 
-	/* Step 5: Kick optimizer again if needed. But if there is a flush requested, */
-	if (completion_done(&optimizer_completion))
-		complete(&optimizer_completion);
+	/* Step 5: Wake up flushers, and kick optimizer again if needed. */
+	optimizer_passes++;
+	wake_up_var_locked(&optimizer_passes, &kprobe_mutex);
 
 	if (!list_empty(&optimizing_list) || !list_empty(&unoptimizing_list))
 		kick_kprobe_optimizer();	/*normal kick*/
@@ -708,7 +710,8 @@ static void wait_for_kprobe_optimizer_locked(void)
 	lockdep_assert_held(&kprobe_mutex);
 
 	while (!list_empty(&optimizing_list) || !list_empty(&unoptimizing_list)) {
-		init_completion(&optimizer_completion);
+		unsigned long passes = optimizer_passes;
+
 		/*
 		 * Set state to OPTIMIZER_ST_FLUSHING and wake up the thread if it's
 		 * idle. If it's already kicked, it will see the state change.
@@ -717,9 +720,12 @@ static void wait_for_kprobe_optimizer_locked(void)
 			OPTIMIZER_ST_FLUSHING) != OPTIMIZER_ST_FLUSHING)
 			wake_up(&kprobe_optimizer_wait);
 
-		mutex_unlock(&kprobe_mutex);
-		wait_for_completion(&optimizer_completion);
-		mutex_lock(&kprobe_mutex);
+		/*
+		 * kprobe_optimizer() holds 'kprobe_mutex' for a whole pass, which
+		 * this drops while sleeping, so a new count means a full pass ran.
+		 */
+		wait_var_event_mutex(&optimizer_passes,
+				     optimizer_passes != passes, &kprobe_mutex);
 	}
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__this_cpu_write`
- 外部函数：`aggr_pre_handler`
- 外部函数：`arch_prepared_optinsn`
- 外部函数：`arch_remove_kprobe`
- 外部函数：`arch_remove_optimized_kprobe`
- 外部函数：`arch_within_optimized_kprobe`
- 外部函数：`atomic_cmpxchg`
- 外部函数：`complete`
- 外部函数：`completion_done`
- 外部函数：`container_of`
- 外部函数：`hash_ptr`
- 外部函数：`init_completion`
- 外部函数：`kfree`
- 外部函数：`kprobe_disabled`
- 外部函数：`likely`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_is_held`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`optimize_kprobe`
- 外部函数：`pre_handler`
- 外部函数：`unlikely`
- 外部函数：`wait_for_completion`
- 外部函数：`wake_up`
- 大写宏：`DECLARE_COMPLETION`
- 大写宏：`MAX_OPTIMIZED_LENGTH`
- 大写宏：`NULL`
- 大写宏：`OPTIMIZER_ST_FLUSHING`
- 大写宏：`OPTIMIZER_ST_IDLE`
- 大写宏：`OPTIMIZER_ST_KICKED`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`But`
- 外部类型：`Don`
- 外部类型：`If`
- 外部类型：`Kick`
- 外部类型：`Now`
- 外部类型：`Set`
- 外部类型：`Step`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`hlist_head`
- 外部类型：`kprobe`
- 外部类型：`kprobe_opcode_t`
- 外部类型：`optimized_kprobe`
- 外部类型：`pt_regs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f80d2efac7` → 本草稿移入 `cases/defect/auto-linux-f80d2efac7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
