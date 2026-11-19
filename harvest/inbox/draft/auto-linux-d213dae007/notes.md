# auto-linux-d213dae007

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#cea5f91daa3ac86c98b63b3a2e89da622670ccf6](https://github.com/torvalds/linux/commit/cea5f91daa3ac86c98b63b3a2e89da622670ccf6) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cea5f91daa3ac86c98b63b3a2e89da622670ccf6 (https://github.com/torvalds/linux/commit/cea5f91daa3ac86c98b63b3a2e89da622670ccf6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 308；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cea5f91daa3ac86c98b63b3a2e89da622670ccf6 wait-event: clean up the internal helper arguments :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -299,13 +299,13 @@ extern void init_wait_entry(struct wait_queue_entry *wq_entry, int flags);
  * otherwise.
  */
 
-#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)		\
+#define ___wait_event(wq_head, condition, state, wq_flag, ret, cmd)		\
 ({										\
 	__label__ __out;							\
 	struct wait_queue_entry __wq_entry;					\
 	long __ret = ret;	/* explicit shadow */				\
 										\
-	init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0);	\
+	init_wait_entry(&__wq_entry, wq_flag);					\
 	for (;;) {								\
 		long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);\
 										\
@@ -327,8 +327,7 @@ __out:	__ret;									\
 })
 
 #define __wait_event(wq_head, condition)					\
-	(void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
-			    schedule())
+	(void)__wait_event_state(wq_head, condition, TASK_UNINTERRUPTIBLE)
 
 /**
  * wait_event - sleep until a condition gets true
@@ -366,8 +365,7 @@ do {										\
 } while (0)
 
 #define __wait_event_freezable(wq_head, condition)				\
-	___wait_event(wq_head, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE),	\
-			0, 0, schedule())
+	__wait_event_state(wq_head, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE))
 
 /**
  * wait_event_freezable - sleep (or freeze) until a condition gets true
@@ -442,8 +440,8 @@ do {										\
 })
 
 #define __wait_event_exclusive_cmd(wq_head, condition, cmd1, cmd2)		\
-	(void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 1, 0,	\
-			    cmd1; schedule(); cmd2)
+	(void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE,		\
+			    WQ_FLAG_EXCLUSIVE, 0, cmd1; schedule(); cmd2)
 /*
  * Just like wait_event_cmd(), except it sets exclusive flag
  */
@@ -480,8 +478,7 @@ do {										\
 } while (0)
 
 #define __wait_event_interruptible(wq_head, condition)				\
-	___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0,		\
-		      schedule())
+	__wait_event_state(wq_head, condition, TASK_INTERRUPTIBLE)
 
 /**
  * wait_event_interruptible - sleep until a condition gets true
@@ -620,8 +617,7 @@ do {										\
 })
 
 #define __wait_event_interruptible_exclusive(wq, condition)			\
-	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 1, 0,			\
-		      schedule())
+	__wait_event_state_exclusive(wq, condition, TASK_INTERRUPTIBLE)
 
 #define wait_event_interruptible_exclusive(wq, condition)			\
 ({										\
@@ -633,8 +629,7 @@ do {										\
 })
 
 #define __wait_event_killable_exclusive(wq, condition)				\
-	___wait_event(wq, condition, TASK_KILLABLE, 1, 0,			\
-		      schedule())
+	__wait_event_state_exclusive(wq, condition, TASK_KILLABLE)
 
 #define wait_event_killable_exclusive(wq, condition)				\
 ({										\
@@ -647,8 +642,7 @@ do {										\
 
 
 #define __wait_event_freezable_exclusive(wq, condition)				\
-	___wait_event(wq, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE), 1, 0,\
-			schedule())
+	__wait_event_state_exclusive(wq, condition, (TASK_INTERRUPTIBLE|TASK_FREEZABLE))
 
 #define wait_event_freezable_exclusive(wq, condition)				\
 ({										\
@@ -676,7 +670,7 @@ do {										\
 do {										\
 	might_sleep();								\
 	if (!(condition))							\
-		___wait_event(wq_head, condition, TASK_IDLE, 0, 0, schedule());	\
+		__wait_event_state(wq_head, condition, TASK_IDLE);		\
 } while (0)
 
 /**
@@ -700,7 +694,7 @@ do {										\
 do {										\
 	might_sleep();								\
 	if (!(condition))							\
-		___wait_event(wq_head, condition, TASK_IDLE, 1, 0, schedule());	\
+		__wait_event_state_exclusive(wq_head, condition, TASK_IDLE);	\
 } while (0)
 
 #define __wait_event_idle_timeout(wq_head, condition, timeout)			\
@@ -738,7 +732,7 @@ do {										\
 
 #define __wait_event_idle_exclusive_timeout(wq_head, condition, timeout)	\
 	___wait_event(wq_head, ___wait_cond_timeout(condition),			\
-		      TASK_IDLE, 1, timeout,					\
+		      TASK_IDLE, WQ_FLAG_EXCLUSIVE, timeout,			\
 		      __ret = schedule_timeout(__ret))
 
 /**
@@ -911,7 +905,7 @@ extern int do_wait_intr_irq(wa
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`___wait_cond_timeout`
- 外部函数：`___wait_event`
- 外部函数：`__wait_event`
- 外部函数：`__wait_event_exclusive_cmd`
- 外部函数：`__wait_event_freezable`
- 外部函数：`__wait_event_freezable_exclusive`
- 外部函数：`__wait_event_idle_exclusive_timeout`
- 外部函数：`__wait_event_idle_timeout`
- 外部函数：`__wait_event_interruptible`
- 外部函数：`__wait_event_interruptible_exclusive`
- 外部函数：`__wait_event_killable`
- 外部函数：`__wait_event_killable_exclusive`
- 外部函数：`__wait_event_state_exclusive`
- 外部函数：`init_wait_entry`
- 外部函数：`might_sleep`
- 外部函数：`prepare_to_wait_event`
- 外部函数：`schedule`
- 外部函数：`schedule_timeout`
- 外部函数：`sleep`
- 外部函数：`wait_event_cmd`
- 外部函数：`wait_event_freezable_exclusive`
- 外部函数：`wait_event_interruptible_exclusive`
- 外部函数：`wait_event_killable_exclusive`
- 外部函数：`wait_event_state_exclusive`
- 大写宏：`TASK_FREEZABLE`
- 大写宏：`TASK_IDLE`
- 大写宏：`TASK_INTERRUPTIBLE`
- 大写宏：`TASK_KILLABLE`
- 大写宏：`TASK_UNINTERRUPTIBLE`
- 外部类型：`Just`
- 外部类型：`list_head`
- 外部类型：`wait_queue_func_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d213dae007` → 本草稿移入 `cases/defect/auto-linux-d213dae007/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
