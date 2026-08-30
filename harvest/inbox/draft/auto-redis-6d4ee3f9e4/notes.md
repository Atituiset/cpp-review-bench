# auto-redis-6d4ee3f9e4

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15532 (https://github.com/redis/redis/pull/15532)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 161；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1,7 +1,7 @@
 #include <stdint.h>
 #include <stdio.h>
 #include <strings.h>
-#if defined(__i386__) || defined(__X86_64__)
+#if defined(__i386__) || defined(__X86_64__) || defined(__x86_64__)
 #include <immintrin.h>
 #endif
 #include "crccombine.h"
@@ -38,9 +38,11 @@
   madler@alumni.caltech.edu
 */
 
-#define STATIC_ASSERT(VVV) do {int test = 1 / (VVV);test++;} while (0)
+#ifndef static_assert
+#define static_assert(expr, lit) extern char __static_assert_failure[(expr) ? 1 : -1]
+#endif
 
-#if !((defined(__i386__) || defined(__X86_64__)))
+#if !((defined(__i386__) || defined(__X86_64__) || defined(__x86_64__)))
 
 /* This cuts 40% of the time vs bit-by-bit. */
 
@@ -115,6 +117,11 @@ uint64_t gf2_matrix_times_switch(uint64_t *mat, uint64_t vec) {
 
 #else
 
+static_assert(sizeof(uint64_t) == 8,
+              "CRC vector implementation requires uint64_t to be 8 bytes");
+static_assert(sizeof(long long unsigned int) == 8,
+              "CRC vector implementation requires unsigned long long to be 8 bytes");
+
 /*
 	Warning: here there be dragons involving vector math, and macros to save us
 	from repeating the same information over and over.
@@ -158,8 +165,6 @@ uint64_t gf2_matrix_times_vec2(uint64_t *mat, uint64_t vec) {
 	DO_CHUNK16();
 	DO_CHUNK16();
 
-	STATIC_ASSERT(sizeof(uint64_t) == 8);
-	STATIC_ASSERT(sizeof(long long unsigned int) == 8);
 	return sum[0] ^ sum[1];
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-6d4ee3f9e4` → 本草稿移入 `cases/defect/auto-redis-6d4ee3f9e4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
