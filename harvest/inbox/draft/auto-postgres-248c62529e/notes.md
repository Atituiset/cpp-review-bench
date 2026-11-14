# auto-postgres-248c62529e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#70c4a57b64d39c526eb4c44a925eb92a6e56f3c5](https://github.com/postgres/postgres/commit/70c4a57b64d39c526eb4c44a925eb92a6e56f3c5) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 62 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #70c4a57b64d39c526eb4c44a925eb92a6e56f3c5 (https://github.com/postgres/postgres/commit/70c4a57b64d39c526eb4c44a925eb92a6e56f3c5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 222；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 70c4a57b64d39c526eb4c44a925eb92a6e56f3c5 Drop support for named POSIX semaphores (the sem_open() kind). :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3,16 +3,13 @@
  * posix_sema.c
  *	  Implement PGSemaphores using POSIX semaphore facilities
  *
- * We prefer the unnamed style of POSIX semaphore (the kind made with
- * sem_init).  We can cope with the kind made with sem_open, however.
+ * We use the unnamed style of POSIX semaphore (the kind made with
+ * sem_init).  We used to support the kind made with sem_open, but
+ * that has several disadvantages, and there are no longer any
+ * platforms where it is a preferable choice.
  *
- * In either implementation, typedef PGSemaphore is equivalent to "sem_t *".
- * With unnamed semaphores, the sem_t structs live in an array in shared
- * memory.  With named semaphores, that's not true because we cannot persuade
- * sem_open to do its allocation there.  Therefore, the named-semaphore code
- * *does not cope with EXEC_BACKEND*.  The sem_t structs will just be in the
- * postmaster's private memory, where they are successfully inherited by
- * forked backends, but they could not be accessed by exec'd backends.
+ * In this implementation, typedef PGSemaphore is equivalent to "sem_t *".
+ * The sem_t structs live in an array in shared memory.
  *
  *
  * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
@@ -25,23 +22,14 @@
  */
 #include "postgres.h"
 
-#include <fcntl.h>
 #include <semaphore.h>
-#include <signal.h>
-#include <unistd.h>
-#include <sys/stat.h>
 
 #include "miscadmin.h"
 #include "storage/ipc.h"
 #include "storage/pg_sema.h"
 #include "storage/shmem.h"
 
 
-/* see file header comment */
-#if defined(USE_NAMED_POSIX_SEMAPHORES) && defined(EXEC_BACKEND)
-#error cannot use named POSIX semaphores with EXEC_BACKEND
-#endif
-
 typedef union SemTPadded
 {
 	sem_t		pgsem;
@@ -56,79 +44,15 @@ typedef struct PGSemaphoreData
 
 #define PG_SEM_REF(x)	(&(x)->sem_padded.pgsem)
 
-#define IPCProtection	(0600)	/* access/modify by user only */
 
-#ifdef USE_NAMED_POSIX_SEMAPHORES
-static sem_t **mySemPointers;	/* keep track of created semaphores */
-#else
 static PGSemaphore sharedSemas; /* array of PGSemaphoreData in shared memory */
-#endif
 static int	numSems;			/* number of semas acquired so far */
-static int	maxSems;			/* allocated size of above arrays */
-
-#ifdef USE_NAMED_POSIX_SEMAPHORES
-static int	nextSemKey;			/* next name to try */
-#endif
+static int	maxSems;			/* allocated size of above array */
 
 
 static void ReleaseSemaphores(int status, Datum arg);
 
 
-#ifdef USE_NAMED_POSIX_SEMAPHORES
-
-/*
- * PosixSemaphoreCreate
- *
- * Attempt to create a new named semaphore.
- *
- * If we fail with a failure code other than collision-with-existing-sema,
- * print out an error and abort.  Other types of errors suggest nonrecoverable
- * problems.
- */
-static sem_t *
-PosixSemaphoreCreate(void)
-{
-	int			semKey;
-	char		semname[64];
-	sem_t	   *mySem;
-
-	for (;;)
-	{
-		semKey = nextSemKey++;
-
-		snprintf(semname, sizeof(semname), "/pgsql-%d", semKey);
-
-		mySem = sem_open(semname, O_CREAT | O_EXCL,
-						 (mode_t) IPCProtection, (unsigned) 1);
-
-#ifdef SEM_FAILED
-		if (mySem != (sem_t *) SEM_FAILED)
-			break;
-#else
-		if (mySem != (sem_t *) (-1))
-			break;
-#endif
-
-		/* Loop if error indicates a collision */
-		if (errno == EEXIST || errno == EACCES || errno == EINTR)
-			continue;
-
-		/*
-		 * Else complain and abort
-		 */
-		elog(FATAL, "sem_open(\"%s\") failed: %m", semname);
-	}
-
-	/*
-	 * Unlink the semaphore immediately, so it can't be accessed externally.
-	 * This also ensures that it will go away if we crash.
-	 */
-	sem_unlink(semname);
-
-	return mySem;
-}
-#else							/* !USE_NAMED_POSIX_SEMAPHORES */
-
 /*
  * PosixSemaphoreCreate
  *
@@ -140,7 +64,6 @@ PosixSemaphoreCreate(sem_t *sem)
 	if (sem_init(sem, 1, 1) < 0)
 		elog(FATAL, "sem_init failed: %m");
 }
-#endif							/* USE_NAMED_POSIX_SEMAPHORES */
 
 
 /*
@@ -149,15 +72,8 @@ PosixSemaphoreCreate(sem_t *sem)
 static void
 PosixSemaphoreKill(sem_t *sem)
 {
-#ifdef USE_NAMED_POSIX_SEMAPHORES
-	/* Got to use sem_close f
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`defined`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`free`
- 外部函数：`malloc`
- 外部函数：`mul_size`
- 外部函数：`sem_close`
- 外部函数：`sem_init`
- 外部函数：`sem_open`
- 外部函数：`sem_unlink`
- 外部函数：`semaphore`
- 外部函数：`snprintf`
- 外部函数：`stat`
- 大写宏：`EACCES`
- 大写宏：`EEXIST`
- 大写宏：`EINTR`
- 大写宏：`EXEC_BACKEND`
- 大写宏：`FATAL`
- 大写宏：`LOG`
- 大写宏：`NULL`
- 大写宏：`O_CREAT`
- 大写宏：`O_EXCL`
- 大写宏：`PANIC`
- 大写宏：`POSIX`
- 大写宏：`SEM_FAILED`
- 大写宏：`USE_NAMED_POSIX_SEMAPHORES`
- 大写宏：`USE_UNNAMED_POSIX_SEMAPHORES`
- 外部类型：`Attempt`
- 外部类型：`DataDir`
- 外部类型：`Datum`
- 外部类型：`Development`
- 外部类型：`Else`
- 外部类型：`For`
- 外部类型：`Global`
- 外部类型：`Got`
- 外部类型：`Group`
- 外部类型：`IPCProtection`
- 外部类型：`If`
- 外部类型：`Implement`
- 外部类型：`In`
- 外部类型：`Loop`
- 外部类型：`Need`
- 外部类型：`No`
- 外部类型：`Other`
- 外部类型：`PGSemaphore`
- 外部类型：`PGSemaphoreData`
- 外部类型：`PGSemaphores`
- 外部类型：`Portions`
- 外部类型：`Posix`
- 外部类型：`PosixSemaphoreCreate`
- 外部类型：`PostgreSQL`
- 外部类型：`SemTPadded`
- 外部类型：`Semaphores`
- 外部类型：`The`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Unlink`
- 外部类型：`We`
- 外部类型：`With`
- 外部类型：`mode_t`
- 外部类型：`sem_t`
- 外部类型：`stat`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-248c62529e` → 本草稿移入 `cases/defect/auto-postgres-248c62529e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
