# auto-postgres-d8146543ae

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d9b5a63f49d9d372a6609243eb0e9417a0a35e6f](https://github.com/postgres/postgres/commit/d9b5a63f49d9d372a6609243eb0e9417a0a35e6f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d9b5a63f49d9d372a6609243eb0e9417a0a35e6f (https://github.com/postgres/postgres/commit/d9b5a63f49d9d372a6609243eb0e9417a0a35e6f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 698；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d9b5a63f49d9d372a6609243eb0e9417a0a35e6f Better express platform requirements in s_lock.h. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -44,23 +44,14 @@
  *		atomic test-and-set only when it appears free.
  *
  *	TAS() and TAS_SPIN() are NOT part of the API, and should never be called
- *	directly.
- *
- *	CAUTION: on some platforms TAS() and/or TAS_SPIN() may sometimes report
- *	failure to acquire a lock even when the lock is not locked.  For example,
- *	on Alpha TAS() will "fail" if interrupted.  Therefore a retry loop must
- *	always be used, even if you are certain the lock is free.
+ *	directly.  A platform must provide either S_LOCK() or a TAS() for the
+ *	default S_LOCK() to be built on.  Currently, all supported platforms do
+ *	the latter, so that is probably the best place to start if adding a new
+ *	one.
  *
  *	It is the responsibility of these macros to make sure that the compiler
  *	does not re-order accesses to shared memory to precede the actual lock
- *	acquisition, or follow the lock release.  Prior to PostgreSQL 9.5, this
- *	was the caller's responsibility, which meant that callers had to use
- *	volatile-qualified pointers to refer to both the spinlock itself and the
- *	shared data being accessed within the spinlocked critical section.  This
- *	was notationally awkward, easy to forget (and thus error-prone), and
- *	prevented some useful compiler optimizations.  For these reasons, we
- *	now require that the macros themselves prevent compiler re-ordering,
- *	so that the caller doesn't need to take special precautions.
+ *	acquisition, or follow the lock release.
  *
  *	On platforms with weak memory ordering, the TAS(), TAS_SPIN(), and
  *	S_UNLOCK() macros must further include hardware-level memory fence
@@ -72,7 +63,7 @@
  *
  *	On most supported platforms, TAS() uses a tas() function written
  *	in assembly language to execute a hardware atomic-test-and-set
- *	instruction.  Equivalent OS-supplied mutex routines could be used too.
+ *	instruction.  Equivalent compiler intrinsics are another popular option.
  *
  *
  * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
@@ -642,19 +633,23 @@ spin_delay(void)
 #endif	/* !defined(TAS) */
 
 
-/* Blow up if we didn't have any way to do spinlocks */
-#ifndef TAS
-#error PostgreSQL does not have spinlock support on this platform.  Please report this to pgsql-bugs@lists.postgresql.org.
-#endif
-
-
 /*
  * Default Definitions - override these above as needed.
  */
 
+/*
+ * Make sure S_LOCK is defined, either explicitly for the platform or via a TAS
+ * macro for the platform.
+ */
 #if !defined(S_LOCK)
+#ifdef TAS
+#define USE_DEFAULT_S_LOCK
+extern int s_lock(volatile slock_t *lock, const char *file, int line, const char *func);
 #define S_LOCK(lock) \
 	(TAS(lock) ? s_lock((lock), __FILE__, __LINE__, __func__) : 0)
+#else
+#error PostgreSQL does not have spinlock support on this platform.  Please report this to pgsql-bugs@lists.postgresql.org.
+#endif	 /* TAS */
 #endif	 /* S_LOCK */
 
 #if !defined(S_UNLOCK)
@@ -687,15 +682,18 @@ extern void s_unlock(volatile slock_t *lock);
 #define SPIN_DELAY()	((void) 0)
 #endif	 /* SPIN_DELAY */
 
-#if !defined(TAS_SPIN)
+/*
+ * TAS_SPIN is only needed by the default S_LOCK's helper function (s_lock()),
+ * so we only provide a default when there is a TAS to base it on.
+ */
+#if !defined(TAS_SPIN) && defined(TAS)
 #define TAS_SPIN(lock)	TAS(lock)
-#endif	 /* TAS_SPIN */
+#endif	 /* ! TAS_SPIN && TAS */
 
 
 /*
  * Platform-independent out-of-line support routines
  */
-extern int s_lock(volatile slock_t *lock, const char *file, int line, const char *func);
 
 /* Support for dynamic adjustment of spins_per_delay */
 #define DEFAULT_SPINS_PER_DELAY  100
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__sync_lock_release`
- 外部函数：`__volatile__`
- 外部函数：`defined`
- 外部函数：`forget`
- 外部函数：`s_lock`
- 大写宏：`API`
- 大写宏：`CAUTION`
- 大写宏：`NOT`
- 外部类型：`Alpha`
- 外部类型：`Blow`
- 外部类型：`Default`
- 外部类型：`Definitions`
- 外部类型：`Development`
- 外部类型：`Equivalent`
- 外部类型：`For`
- 外部类型：`Global`
- 外部类型：`Group`
- 外部类型：`It`
- 外部类型：`Note`
- 外部类型：`Nowadays`
- 外部类型：`On`
- 外部类型：`Platform`
- 外部类型：`Please`
- 外部类型：`Portions`
- 外部类型：`PostgreSQL`
- 外部类型：`Prior`
- 外部类型：`Support`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Use`
- 外部类型：`When`
- 外部类型：`Without`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d8146543ae` → 本草稿移入 `cases/defect/auto-postgres-d8146543ae/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
