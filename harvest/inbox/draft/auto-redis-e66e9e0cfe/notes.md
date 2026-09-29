# auto-redis-e66e9e0cfe

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15710 (https://github.com/redis/redis/pull/15710)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 140；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -13,6 +13,11 @@ monotime (*getMonotonicUs)(void) = NULL;
 
 static char monotonic_info_string[32];
 
+/* Optional log callback, set via monotonicInit(). */
+static void (*monotonic_logger)(const char *fmt, ...) __attribute__((format(printf, 1, 2))) = NULL;
+#define monotonicLog(...) do { \
+    if (monotonic_logger) monotonic_logger(__VA_ARGS__); \
+} while (0)
 
 /* Using the processor clock (aka TSC on x86) can provide improved performance
  * throughout Redis wherever the monotonic clock is used.  The processor clock
@@ -137,7 +142,7 @@ static void monotonicInit_x86linux(void) {
     FILE *cs = fopen("/sys/devices/system/clocksource/clocksource0/current_clocksource", "r");
     if (cs == NULL || fgets(buf, bufflen, cs) == NULL || strncmp(buf, "tsc", 3) != 0) {
         if (cs) fclose(cs);
-        fprintf(stderr, "monotonic: x86 linux, kernel clocksource is not 'tsc'\n");
+        monotonicLog("x86 linux, kernel clocksource is not 'tsc'");
         return;
     }
     fclose(cs);
@@ -176,7 +181,7 @@ static void monotonicInit_x86linux(void) {
     regfree(&constTscRegex);
 
     if (!constantTsc) {
-        fprintf(stderr, "monotonic: x86 linux, 'constant_tsc' flag not present\n");
+        monotonicLog("x86 linux, 'constant_tsc' flag not present");
         return;
     }
 
@@ -211,9 +216,9 @@ static void monotonicInit_x86linux(void) {
         if (measured > 0 && labs(measured - nominal_model) * 1000 <= nominal_model) { /* within 0.1% */
             mono_ticksPerMicrosecond = nominal_model;
         } else {
-            fprintf(stderr, "monotonic: x86 linux, advertised clock rate "
+            monotonicLog("x86 linux, advertised clock rate "
                     "(%ld ticks/us) unconfirmed by the measured rate "
-                    "(%ld ticks/us), using calibration\n",
+                    "(%ld ticks/us), using calibration",
                     nominal_model, measured);
         }
     }
@@ -243,7 +248,7 @@ static void monotonicInit_x86linux(void) {
     }
 
     if (mono_ticksPerMicrosecond == 0) {
-        fprintf(stderr, "monotonic: x86 linux, unable to determine clock rate\n");
+        monotonicLog("x86 linux, unable to determine clock rate");
         return;
     }
 
@@ -282,7 +287,7 @@ static monotime getMonotonicUs_aarch64(void) {
 static void monotonicInit_aarch64(void) {
     mono_ticksPerMicrosecond = (long)cntfrq_hz() / 1000L / 1000L;
     if (mono_ticksPerMicrosecond == 0) {
-        fprintf(stderr, "monotonic: aarch64, unable to determine clock rate\n");
+        monotonicLog("aarch64, unable to determine clock rate");
         return;
     }
 
@@ -339,7 +344,7 @@ static monotime getMonotonicUs_riscv(void) {
 static void monotonicInit_riscv(void) {
     mono_ticksPerMicrosecond = (long)get_timebase_frequency() / 1000L / 1000L;
     if (mono_ticksPerMicrosecond == 0) {
-        fprintf(stderr, "monotonic: riscv, unable to determine clock rate\n");
+        monotonicLog("riscv, unable to determine clock rate");
         return;
     }
     snprintf(monotonic_info_string, sizeof(monotonic_info_string),
@@ -373,7 +378,9 @@ static void monotonicInit_posix(void) {
 
 
 
-const char * monotonicInit(void) {
+const char * monotonicInit(void (*logger)(const char *fmt, ...)) {
+    if (getMonotonicUs == NULL) monotonic_logger = logger;
+
     #if defined(__x86_64__) && defined(__linux__)
     if (getMonotonicUs == NULL) monotonicInit_x86linux();
     #endif
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e66e9e0cfe` → 本草稿移入 `cases/defect/auto-redis-e66e9e0cfe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
