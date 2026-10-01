# auto-curl-f40a53943a

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1848 (https://github.com/curl/curl/pull/1848)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -44,7 +44,9 @@
 #include "rand.h"
 #include "vtls/vtls.h"
 
-#ifdef USE_NSS
+/* SSL backend-specific #if branches in this file must be kept in the order
+   documented in curl_ntlm_core. */
+#if defined(NTLM_NEEDS_NSS_INIT)
 #include "vtls/nssg.h" /* for Curl_nss_force_init() */
 #endif
 
@@ -272,7 +274,7 @@ CURLcode Curl_auth_decode_ntlm_type2_message(struct Curl_easy *data,
   unsigned char *type2 = NULL;
   size_t type2_len = 0;
 
-#if defined(USE_NSS)
+#if defined(NTLM_NEEDS_NSS_INIT)
   /* Make sure the crypto backend is initialized */
   result = Curl_nss_force_init(data);
   if(result)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-f40a53943a` → 本草稿移入 `cases/defect/auto-curl-f40a53943a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
