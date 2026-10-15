# auto-curl-93191027d2

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1848 (https://github.com/curl/curl/pull/1848)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -41,7 +41,9 @@
 #include "vauth/vauth.h"
 #include "url.h"
 
-#if defined(USE_NSS)
+/* SSL backend-specific #if branches in this file must be kept in the order
+   documented in curl_ntlm_core. */
+#if defined(NTLM_NEEDS_NSS_INIT)
 #include "vtls/nssg.h"
 #elif defined(USE_WINDOWS_SSPI)
 #include "curl_sspi.h"
@@ -129,7 +131,7 @@ CURLcode Curl_output_ntlm(struct connectdata *conn, bool proxy)
   DEBUGASSERT(conn);
   DEBUGASSERT(conn->data);
 
-#ifdef USE_NSS
+#if defined(NTLM_NEEDS_NSS_INIT)
   if(CURLE_OK != Curl_nss_force_init(conn->data))
     return CURLE_OUT_OF_MEMORY;
 #endif
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-93191027d2` → 本草稿移入 `cases/defect/auto-curl-93191027d2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
