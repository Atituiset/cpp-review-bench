# auto-redis-21a2513579

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15891](https://github.com/redis/redis/pull/15891) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 317 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15891 (https://github.com/redis/redis/pull/15891)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 2910；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15891 Bound a stuck crash report so the process still exits. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -60,6 +60,100 @@ static pthread_mutexattr_t bug_report_start_attr;
 static pthread_mutex_t signal_handler_lock;
 static pthread_mutexattr_t signal_handler_lock_attr;
 static volatile int signal_handler_lock_initialized = 0;
+
+/* Crash watchdog: bounds a bug report so a stuck backtrace() or module info
+ * callback still exits. */
+/* Seconds a bug report may run; tests shorten it with
+ * DEBUG SET-CRASH-WATCHDOG-TIMEOUT. */
+static int crash_watchdog_timeout = 30;
+#ifdef HAVE_CRASH_WATCHDOG
+/* SIGRTMIN+4: glibc reserves +0..+2, and SIGALRM is the software watchdog. */
+#define CRASH_WATCHDOG_SIG (SIGRTMIN + 4)
+static timer_t crash_watchdog_timer;
+static int crash_watchdog_ok = 0;   /* the timer was created */
+
+/* The crash handler calls functions that are not async-signal-safe, like
+ * backtrace(), and we suspect they can deadlock. If the bug report doesn't
+ * finish in time, abort() so the process still terminates in a bounded time
+ * and a core dump is generated. Use abort(), not kill(): a signal sent to
+ * ourselves can be dropped. */
+static void crashWatchdogHandler(int sig) {
+    UNUSED(sig);
+    struct sigaction act;
+
+    /* No bug report running: a stray signal must not kill the server. */
+    if (!bug_report_start) return;
+
+    serverLogRawFromHandler(LL_WARNING|LL_RAW,
+"\n=== CRASH WATCHDOG: crash report exceeded its timeout ===\n"
+"    The bug report above is incomplete. Aborting so the process can be\n"
+"    restarted and a core dump produced.\n\n");
+
+    /* All of them, not only SIGABRT: glibc's fallback fault must not land
+     * back in sigsegvHandler(). */
+    sigemptyset(&act.sa_mask);
+    act.sa_flags = 0;
+    act.sa_handler = SIG_DFL;
+    sigaction(SIGSEGV, &act, NULL);
+    sigaction(SIGBUS, &act, NULL);
+    sigaction(SIGFPE, &act, NULL);
+    sigaction(SIGILL, &act, NULL);
+    sigaction(SIGABRT, &act, NULL);
+    abort();
+}
+#endif
+
+/* Called at startup and in fork children (POSIX timers are not inherited). */
+void crashWatchdogInit(void) {
+#ifdef HAVE_CRASH_WATCHDOG
+    struct sigaction act;
+    struct sigevent sev;
+    sigset_t set;
+
+    /* Threads inherit the mask, and we may have been started with it blocked. */
+    sigemptyset(&set);
+    sigaddset(&set, CRASH_WATCHDOG_SIG);
+    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
+
+    sigemptyset(&act.sa_mask);
+    /* A stray delivery must not return EINTR from unrelated syscalls. */
+    act.sa_flags = SA_RESTART;
+    act.sa_handler = crashWatchdogHandler;
+
+    memset(&sev, 0, sizeof(sev));
+    sev.sigev_notify = SIGEV_SIGNAL;
+    sev.sigev_signo = CRASH_WATCHDOG_SIG;
+
+    /* Without the flag, crashWatchdogSet() would use an unset timer_t, which
+     * can name a timer someone else created. */
+    crash_watchdog_ok = sigaction(CRASH_WATCHDOG_SIG, &act, NULL) == 0 &&
+                        timer_create(CLOCK_MONOTONIC, &sev, &crash_watchdog_timer) == 0;
+    if (!crash_watchdog_ok)
+        serverLog(LL_WARNING, "Crash watchdog disabled: %s", strerror(errno));
+#endif
+}
+
+/* Arm the deadline, or disarm it with 0. */
+static void crashWatchdogSet(int seconds) {
+#ifdef HAVE_CRASH_WATCHDOG
+    if (!crash_watchdog_ok) return;
+    struct itimerspec its = {{0, 0}, {seconds, 0}};
+    timer_settime(crash_watchdog_timer, 0, &its, NULL);
+#else
+    UNUSED(seconds);
+#endif
+}
+
+/* Arm the deadline unless it is already counting down. */
+static void crashWatchdogArmIfIdle(void) {
+#ifdef HAVE_CRASH_WATCHDOG
+    struct itimerspec cur;
+    if (crash_watchdog_ok && timer_gettime(crash_watchdog_timer, &cur) == 0 &&
+        (cur.it_value.tv_sec || cur.it_value.tv_nsec)) return;
+#endif
+    crashWatchdogSet(crash_watchdog_timeout);
+}
+
 /* Forward declarations */
 int bugReportStart(void);
 void printCrashReport(void);
@@ -511,6 +605,8 @@ void debugCommand(client *c) {
 "    Setting it to 0 prevents access to expired keys (and hash-fields),",
 "    simulating the standard Redis behavior. Setting it to 1 al
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_exit`
- 外部函数：`abort`
- 外部函数：`addReply`
- 外部函数：`arm_thread_state64_get_fp`
- 外部函数：`arm_thread_state64_get_lr`
- 外部函数：`arm_thread_state64_get_pc`
- 外部函数：`arm_thread_state64_get_sp`
- 外部函数：`arm_thread_state64_set_pc_fptr`
- 外部函数：`atoi`
- 外部函数：`atomicSet`
- 外部函数：`backtrace`
- 外部函数：`backtrace_symbols_fd`
- 外部函数：`bioKillThreads`
- 外部函数：`catClientInfoString`
- 外部函数：`close`
- 外部函数：`clusterGenNodesDescription`
- 外部函数：`collect_stacktrace_data`
- 外部函数：`createStringObject`
- 外部函数：`dbFind`
- 外部函数：`decrRefCount`
- 外部函数：`dladdr`
- 外部函数：`fclose`
- 外部函数：`fflush`
- 外部函数：`fgets`
- 外部函数：`fgets_async_signal_safe`
- 外部函数：`field`
- 外部函数：`fopen`
- 外部函数：`free`
- 外部函数：`genClusterInfoString`
- 外部函数：`genInfoSectionDict`
- 外部函数：`genRedisInfoString`
- 外部函数：`getAllClientsInfoString`
- 外部函数：`getConfigDebugInfo`
- 外部函数：`getDecodedObject`
- 外部函数：`getpid`
- 外部函数：`hashTypeLength`
- 外部函数：`keys`
- 外部函数：`kill`
- 外部函数：`killIOThreads`
- 外部函数：`listTypeLength`
- 外部函数：`lock`
- 外部函数：`logStackTrace`
- 外部函数：`lookupCommand`
- 外部函数：`lx`
- 外部函数：`memcpy`
- 外部函数：`memtest_preserving_test`
- 外部函数：`min`
- 外部函数：`modulesCollectInfo`
- 外部函数：`open`
- 外部函数：`prctl`
- 外部函数：`printf`
- 外部函数：`pthread_cancel`
- 外部函数：`pthread_join`
- 外部函数：`pthread_mutex_init`
- 外部函数：`pthread_mutex_lock`
- 外部函数：`pthread_mutexattr_init`
- 外部函数：`pthread_mutexattr_settype`
- 外部函数：`pthread_self`
- 外部函数：`read`
- 外部函数：`redactLogCstr`
- 外部函数：`releaseInfoSectionDict`
- 外部函数：`s`
- 外部函数：`safe`
- 外部函数：`sdsEncodedObject`
- 外部函数：`sdscatprintf`
- 外部函数：`sdscatrepr`
- 外部函数：`sdscatsds`
- 外部函数：`sdsempty`
- 外部函数：`sdsfree`
- 外部函数：`sdslen`
- 外部函数：`serverLog`
- 外部函数：`serverLogFromHandler`
- 外部函数：`serverLogRaw`
- 外部函数：`serverLogRawFromHandler`
- 外部函数：`setTypeSize`
- 外部函数：`sigaction`
- 外部函数：`sigemptyset`
- 外部函数：`sigsegvHandler`
- 外部函数：`snprintf`
- 外部函数：`snprintf_async_signal_safe`
- 外部函数：`strcasecmp`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`streamLength`
- 外部函数：`strerror`
- 外部函数：`string2l`
- 外部函数：`string2ul_base16_async_signal_safe`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strstr`
- 外部函数：`strtoul`
- 外部函数：`syscall`
- 外部函数：`sysconf`
- 外部函数：`unlink`
- 外部函数：`write`
- 外部函数：`writeCurrentThreadsStackTrace`
- 外部函数：`writeStacktraces`
- 外部函数：`zsetLength`
- 大写宏：`AMD64`
- 大写宏：`ARM`
- 大写宏：`ARM64`
- 大写宏：`AROUND`
- 大写宏：`ASAN`
- 大写宏：`ASAP`
- 大写宏：`BUG`
- 大写宏：`CALL`
- 大写宏：`CLIENT`
- 大写宏：`CLUSTER`
- 大写宏：`CODE`
- 大写宏：`CONFIG`
- 大写宏：`CURRENT`
- 大写宏：`DEBUG`
- 大写宏：`DETECTED`
- 大写宏：`DONE`
- 大写宏：`DUMPING`
- 大写宏：`EAX`
- 大写宏：`EBP`
- 大写宏：`EBX`
- 大写宏：`ECX`
- 大写宏：`EDEADLK`
- 大写宏：`EDI`
- 大写宏：`EDX`
- 大写宏：`EFL`
- 大写宏：`EIP`
- 大写宏：`ENABLE_GCRA`
- 大写宏：`END`
- 大写宏：`EOF`
- 大写宏：`ERROR`
- 大写宏：`ESI`
- 大写宏：`ESP`
- 大写宏：`EXECUTING`
- 大写宏：`FAST`
- 大写宏：`FILE`
- 大写宏：`GCRA`
- 大写宏：`HAVE_BACKTRACE`
- 大写宏：`HAVE_PROC_MAPS`
- 大写宏：`IA64`
- 大写宏：`INFO`
- 大写宏：`LIST`
- 大写宏：`LL_NOTICE`
- 大写宏：`LL_RAW`
- 大写宏：`LL_WARNING`
- 大写宏：`MAC_OS_10_6_DETECTED`
- 大写宏：`MEMORY`
- 大写宏：`MODULES`
- 大写宏：`NODES`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`OBJ_ENCODING_SKIPLIST`
- 大写宏：`OBJ_GCRA`
- 大写宏：`OBJ_HASH`
- 大写宏：`OBJ_LIST`
- 大写宏：`OBJ_SET`
- 大写宏：`OBJ_STREAM`
- 大写宏：`OBJ_STRING`
- 大写宏：`OBJ_ZSET`
- 大写宏：`OSX`
- 大写宏：`OUTPUT`
- 大写宏：`O_APPEND`
- 大写宏：`O_CREAT`
- 大写宏：`O_DIRECTORY`
- 大写宏：`O_RDONLY`
- 大写宏：`O_WRONLY`
- 大写宏：`PACKED`
- 大写宏：`PASSED`
- 大写宏：`PATH_MAX`
- 大写宏：`PID`
- 大写宏：`PII`
- 大写宏：`PIPE_BUF`
- 大写宏：`PR_GET_NAME`
- 大写宏：`PTHREAD_MUTEX_ERRORCHECK`
- 大写宏：`PTHREAD_MUTEX_RECURSIVE`
- 大写宏：`QUICKLIST`
- 大写宏：`R10`
- 大写宏：`R11`
- 大写宏：`R12`
- 大写宏：`R13`
- 大写宏：`R15`
- 大写宏：`RAM`
- 大写宏：`RAX`
- 大写宏：`RBX`
- 大写宏：`RDI`
- 大写宏：`RDX`
- 大写宏：`REDIS`
- 大写宏：`REDIS_NO_SANITIZE`
- 大写宏：`REDIS_NO_SANITIZE_MSAN`
- 大写宏：`REDIS_VERSION`
- 大写宏：`REGISTERS`
- 大写宏：`REG_CS`
- 大写宏：`REG_PC`
- 大写宏：`REG_R10`
- 大写宏：`REG_R11`
- 大写宏：`REG_R12`
- 大写宏：`REG_R13`
- 大写宏：`REG_R14`
- 大写宏：`REG_R15`
- 大写宏：`REG_R8`
- 大写宏：`REG_R9`
- 大写宏：`REG_RAX`
- 大写宏：`REG_RBP`
- 大写宏：`REG_RBX`
- 大写宏：`REG_RCX`
- 大写宏：`REG_RDI`
- 大写宏：`REG_RDX`
- 大写宏：`REG_RFL`
- 大写宏：`REG_RIP`
- 大写宏：`REG_RSI`
- 大写宏：`REG_RSP`
- 大写宏：`REG_SP`
- 大写宏：`REPORT`
- 大写宏：`RIP`
- 大写宏：`RISC`
- 大写宏：`RSI`
- 大写宏：`RSP`
- 大写宏：`SA_NODEFER`
- 大写宏：`SA_SIGINFO`
- 大写宏：`SIGABRT`
- 大写宏：`SIGBUS`
- 大写宏：`SIGFPE`
- 大写宏：`SIGILL`
- 大写宏：`SIGSEGV`
- 大写宏：`SIG_DFL`
- 大写宏：`SI_USER`
- 大写宏：`STACK`
- 大写宏：`START`
- 大写宏：`STDOUT_FILENO`
- 大写宏：`TEST`
- 大写宏：`THREADS_SIGNAL`
- 大写宏：`THRESHOLD`
- 大写宏：`TRACE`
- 大写宏：`UID`
- 大写宏：`UINTPTR_MAX`
- 大写宏：`UNSAFE_CRASH_REPORT`
- 大写宏：`UNUSED`
- 大写宏：`X18`
- 大写宏：`X19`
- 大写宏：`X21`
- 大写宏：`X22`
- 大写宏：`X23`
- 大写宏：`X25`
- 大写宏：`X26`
- 大写宏：`X27`
- 大写宏：`X29`
- 大写宏：`X30`
- 外部类型：`AArch64`
- 外部类型：`Accessing`
- 外部类型：`Add`
- 外部类型：`Allow`
- 外部类型：`Attempting`
- 外部类型：`Avoid`
- 外部类型：`Can`
- 外部类型：`Check`
- 外部类型：`Cleanup`
- 外部类型：`Clear`
- 外部类型：`Cluster`
- 外部类型：`Crashed`
- 外部类型：`Default`
- 外部类型：`Dl_info`
- 外部类型：`Don`
- 外部类型：`Dumping`
- 外部类型：`Each`
- 外部类型：`Failed`
- 外部类型：`Fast`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`Forward`
- 外部类型：`FreeBSD`
- 外部类型：`Function`
- 外部类型：`Get`
- 外部类型：`Hash`
- 外部类型：`Hence`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Initialize`
- 外部类型：`It`
- 外部类型：`Killed`
- 外部类型：`Linux`
- 外部类型：`List`
- 外部类型：`Log`
- 外部类型：`LoongArch64`
- 外部类型：`Make`
- 外部类型：`Max`
- 外部类型：`Module`
- 外部类型：`Not`
- 外部类型：`Nothing`
- 外部类型：`Object`
- 外部类型：`Open`
- 外部类型：`OpenBSD`
- 外部类型：`Otherwise`
- 外部类型：`Outside`
- 外部类型：`Please`
- 外部类型：`Preparing`
- 外部类型：`Providing`
- 外部类型：`Reached`
- 外部类型：`Read`
- 外部类型：`Redis`
- 外部类型：`Restore`
- 外部类型：`Run`
- 外部类型：`SYS_getdents64`
- 外部类型：`SYS_gettid`
- 外部类型：`Send`
- 外部类型：`Set`
- 外部类型：`Sets`
- 外部类型：`Setting`
- 外部类型：`SigBlk`
- 外部类型：`SigIgn`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`Skiplist`
- 外部类型：`So`
- 外部类型：`Some`
- 外部类型：`Something`
- 外部类型：`Sorted`
- 外部类型：`Start`
- 外部类型：`Stop`
- 外部类型：`Stream`
- 外部类型：`Suspect`
- 外部类型：`Swap`
- 外部类型：`Symbol`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`ThreadsManager_runOnThreads`
- 外部类型：`To`
- 外部类型：`Use`
- 外部类型：`Using`
- 外部类型：`When`
- 外部类型：`Write`
- 外部类型：`linux_dirent64`
- 外部类型：`pid_t`
- 外部类型：`pthread_mutex_t`
- 外部类型：`pthread_mutexattr_t`
- 外部类型：`redisCommand`
- 外部类型：`sigaction`
- 外部类型：`siginfo_t`
- 外部类型：`size_t`
- 外部类型：`ucontext_t`
- 外部类型：`uint32_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-21a2513579` → 本草稿移入 `cases/defect/auto-redis-21a2513579/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
