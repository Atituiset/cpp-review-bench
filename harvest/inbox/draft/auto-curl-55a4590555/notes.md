# auto-curl-55a4590555

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1379 (https://github.com/curl/curl/pull/1379)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -39,6 +39,9 @@
 #elif defined(HAVE_POLL_H)
 #include <poll.h>
 #endif
+#ifdef __MINGW32__
+#include <w32api.h>
+#endif
 
 #define ENABLE_CURLX_PRINTF
 /* make the curlx header define all printf() functions to use the curlx_*
@@ -55,9 +58,14 @@
 #define EINVAL  22 /* errno.h value */
 #endif
 
+/* MinGW with w32api version < 3.6 declared in6addr_any as extern,
+   but lacked the definition */
 #if defined(ENABLE_IPV6) && defined(__MINGW32__)
+#if (__W32API_MAJOR_VERSION < 3) || \
+    ((__W32API_MAJOR_VERSION == 3) && (__W32API_MINOR_VERSION < 6))
 const struct in6_addr in6addr_any = {{ IN6ADDR_ANY_INIT }};
-#endif
+#endif /* w32api < 3.6 */
+#endif /* ENABLE_IPV6 && __MINGW32__*/
 
 /* This function returns a pointer to STATIC memory. It converts the given
  * binary lump to a hex formatted string usable for output in logs or
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-55a4590555` → 本草稿移入 `cases/defect/auto-curl-55a4590555/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
