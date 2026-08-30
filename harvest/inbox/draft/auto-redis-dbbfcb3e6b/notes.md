# auto-redis-dbbfcb3e6b

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15045 (https://github.com/redis/redis/pull/15045)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -91,6 +91,33 @@ static inline int log2ceil(size_t x) {
 #endif
 }
 
+/* Check for __builtin_add_overflow() */
+#ifndef __has_builtin
+#define __has_builtin(x) 0
+#endif
+#if __has_builtin(__builtin_add_overflow) || (defined(__GNUC__) && __GNUC__ >= 5)
+#define add_overflow_ll(a, b, res) __builtin_add_overflow((a), (b), (res))
+#define sub_overflow_ll(a, b, res) __builtin_sub_overflow((a), (b), (res))
+#else
+#include <limits.h>
+static inline int add_overflow_ll(long long a, long long b, long long *res) {
+    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) {
+        *res = (long long)((unsigned long long)a + (unsigned long long)b);
+        return 1;
+    }
+    *res = a + b;
+    return 0;
+}
+static inline int sub_overflow_ll(long long a, long long b, long long *res) {
+    if ((b < 0 && a > LLONG_MAX + b) || (b > 0 && a < LLONG_MIN + b)) {
+        *res = (long long)((unsigned long long)a - (unsigned long long)b);
+        return 1;
+    }
+    *res = a - b;
+    return 0;
+}
+#endif
+
 #ifndef static_assert
 #define static_assert(expr, lit) extern char __static_assert_failure[(expr) ? 1:-1]
 #endif
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-dbbfcb3e6b` → 本草稿移入 `cases/defect/auto-redis-dbbfcb3e6b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
