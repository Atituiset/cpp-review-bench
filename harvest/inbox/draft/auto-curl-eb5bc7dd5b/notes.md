# auto-curl-eb5bc7dd5b

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #3262 (https://github.com/curl/curl/pull/3262)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1641；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1638,17 +1638,6 @@ static CURLcode nss_load_ca_certificates(struct connectdata *conn,
 static CURLcode nss_sslver_from_curl(PRUint16 *nssver, long version)
 {
   switch(version) {
-  case CURL_SSLVERSION_TLSv1:
-    /* TODO: set sslver->max to SSL_LIBRARY_VERSION_TLS_1_3 once stable */
-#ifdef SSL_LIBRARY_VERSION_TLS_1_2
-    *nssver = SSL_LIBRARY_VERSION_TLS_1_2;
-#elif defined SSL_LIBRARY_VERSION_TLS_1_1
-    *nssver = SSL_LIBRARY_VERSION_TLS_1_1;
-#else
-    *nssver = SSL_LIBRARY_VERSION_TLS_1_0;
-#endif
-    return CURLE_OK;
-
   case CURL_SSLVERSION_SSLv2:
     *nssver = SSL_LIBRARY_VERSION_2;
     return CURLE_OK;
@@ -1709,10 +1698,8 @@ static CURLcode nss_init_sslver(SSLVersionRange *sslver,
   }
 
   switch(min) {
-  case CURL_SSLVERSION_DEFAULT:
-    break;
   case CURL_SSLVERSION_TLSv1:
-    sslver->min = SSL_LIBRARY_VERSION_TLS_1_0;
+  case CURL_SSLVERSION_DEFAULT:
     break;
   default:
     result = nss_sslver_from_curl(&sslver->min, min);
@@ -1792,7 +1779,11 @@ static CURLcode nss_setup_connect(struct connectdata *conn, int sockindex)
 
   SSLVersionRange sslver = {
     SSL_LIBRARY_VERSION_TLS_1_0,  /* min */
-    SSL_LIBRARY_VERSION_TLS_1_0   /* max */
+#ifdef SSL_LIBRARY_VERSION_TLS_1_3
+    SSL_LIBRARY_VERSION_TLS_1_3   /* max */
+#else
+    SSL_LIBRARY_VERSION_TLS_1_2
+#endif
   };
 
   BACKEND->data = data;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-eb5bc7dd5b` → 本草稿移入 `cases/defect/auto-curl-eb5bc7dd5b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
