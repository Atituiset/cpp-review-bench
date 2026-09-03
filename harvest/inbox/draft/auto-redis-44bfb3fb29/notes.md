# auto-redis-44bfb3fb29

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15018 (https://github.com/redis/redis/pull/15018)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 67；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -5,6 +5,8 @@
 #include <time.h>
 #include "redisassert.h"
 #include <string.h>
+#include <errno.h>
+#include <limits.h>
 
 /* The function pointer for clock retrieval.  */
 monotime (*getMonotonicUs)(void) = NULL;
@@ -14,13 +16,47 @@ static char monotonic_info_string[32];
 
 /* Using the processor clock (aka TSC on x86) can provide improved performance
  * throughout Redis wherever the monotonic clock is used.  The processor clock
- * is significantly faster than calling 'clock_getting' (POSIX).  While this is
+ * is significantly faster than calling 'clock_gettime' (POSIX).  While this is
  * generally safe on modern systems, this link provides additional information
  * about use of the x86 TSC: http://oliveryang.net/2015/09/pitfalls-of-TSC-usage
  *
+ * On x86_64 Linux the hardware clock is enabled by default, with two safety
+ * gates and a layered frequency-detection chain.  The reasoning, for future
+ * generations:
+ *
+ * Reliability: rather than replicating the kernel's knowledge of broken TSCs
+ * (known-bad CPU quirk lists, boot-time sync tests, the clocksource watchdog
+ * that demotes a TSC that drifts at runtime), we simply require that the
+ * kernel's ACTIVE clocksource is "tsc".  Machines where Linux distrusts the
+ * TSC never satisfy that, so they transparently stay on the POSIX clock.
+ * 'constant_tsc' in /proc/cpuinfo is additionally required (fixed tick rate
+ * regardless of frequency scaling).
+ *
+ * Speed vs the VDSO: clock_gettime(CLOCK_MONOTONIC) on a tsc clocksource is
+ * a fast VDSO call (no context switch), but it still costs ~2-3x a raw
+ * RDTSC: the seqlock-protected read of the timekeeper data, the mult/shift
+ * conversion, ns scaling and the libc call.  The monotonic clock is read
+ * several times per command, so the difference is measurable end-to-end
+ * once the network stops being the bottleneck (measured on bare-metal
+ * Sapphire Rapids at 1KiB SET/GET, 2000 connections: +7-9% throughput at
+ * 8-16 io-threads; flat at 0-4 io-threads, which are network-bound).
+ *
+ * Tick rate: the frequency advertised in the "model name" cpuinfo string is
+ * the marketing value and can differ from the real TSC rate by a few tenths
+ * of a percent (e.g. a "2.30GHz" part whose TSC ticks at ~2294 MHz) — a rate
+ * error that size skews every measured duration and accumulates as drift.
+ * The kernel measures the true rate at boot, but does not expose it to
+ * userspace on mainline; the tsc_freq_khz sysfs file IS that kernel-measured
+ * value on kernels that carry the patch.  Calibrating RDTSC against
+ * CLOCK_MONOTONIC recovers the same kernel-measured rate indirectly, because
+ * with a tsc clocksource CLOCK_MONOTONIC itself advances at the kernel's
+ * calibrated TSC frequency.  Hence the chain: model-name parse, validated
+ * against one measured sample (calibration wins when they disagree beyond
+ * noise), then tsc_freq_khz, then median-of-3 calibration.
+ *
  * On ARM aarch64 systems, the hardware clock is enabled by default because the
  * ARM Generic Timer is architecturally guaranteed to be available and monotonic
- * on all ARMv8-A processors (see the “The Generic Timer in AArch64 state”
+ * on all ARMv8-A processors (see the "The Generic Timer in AArch64 state"
  * section of the Arm Architecture Reference Manual for Armv8-A).
  *
  * To use the processor clock on other architectures, either uncomment this line,
@@ -30,7 +66,7 @@ static char monotonic_info_string[32];
  */
 
 
-#if defined(USE_PROCESSOR_CLOCK) && defined(__x86_64__) && defined(__linux__)
+#if defined(__x86_64__) && defined(__linux__)
 #include <regex.h>
 #include <x86intrin.h>
 
@@ -40,58 +76,177 @@ static monotime getMonotonicUs_x86(void) {
     return __rdtsc() / mono_ticksPerMicrosecond;
 }
 
+/* One calibration measurement: RDTSC ticks across a ~10ms nanosleep, bounded
+ * by CLOCK_MONOTONIC readings.  Returns ticks-per-microsecond, or 0 on any
+ * failure (clock error, non-monotonic TSC sample pair).  */
+sta
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-44bfb3fb29` → 本草稿移入 `cases/defect/auto-redis-44bfb3fb29/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
