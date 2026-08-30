# auto-redis-340bbbf5a7

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14661 (https://github.com/redis/redis/pull/14661)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 28；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1,32 +0,0 @@
-#include "fast_float.h"
-#include <iostream>
-#include <string>
-#include <system_error>
-#include <cerrno>
-
-/* Convert NPTR to a double using the fast_float library.
- *
- * This function behaves similarly to the standard strtod function, converting
- * the initial portion of the string pointed to by `nptr` to a `double` value,
- * using the fast_float library for high performance. If the conversion fails,
- * errno is set to EINVAL error code.
- *
- * @param nptr   A pointer to the null-terminated byte string to be interpreted.
- * @param endptr A pointer to a pointer to character. If `endptr` is not NULL,
- *               it will point to the character after the last character used
- *               in the conversion.
- * @return       The converted value as a double. If no valid conversion could
- *               be performed, returns 0.0.
- * If ENDPTR is not NULL, a pointer to the character after the last one used
- * in the number is put in *ENDPTR.  */
-extern "C" double fast_float_strtod(const char *nptr, char **endptr) {
-  double result = 0.0;
-  auto answer = fast_float::from_chars(nptr, nptr + strlen(nptr), result);
-  if (answer.ec != std::errc()) {
-    errno = EINVAL;  // Fallback to  for other errors
-  }
-  if (endptr != NULL) {
-    *endptr = (char *)answer.ptr;
-  }
-  return result;
-}
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-340bbbf5a7` → 本草稿移入 `cases/defect/auto-redis-340bbbf5a7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
