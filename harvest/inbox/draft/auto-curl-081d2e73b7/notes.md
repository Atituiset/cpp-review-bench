# auto-curl-081d2e73b7

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #20545 (https://github.com/curl/curl/pull/20545)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 471；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -354,6 +354,8 @@ CURLcode Curl_auth_create_digest_md5_message(struct Curl_easy *data,
   char method[]     = "AUTHENTICATE";
   char qop[]        = DIGEST_QOP_VALUE_STRING_AUTH;
   char *spn         = NULL;
+  char *qrealm;
+  char *qnonce;
 
   /* Decode the challenge message */
   CURLcode result = auth_decode_digest_md5_message(chlg,
@@ -467,12 +469,20 @@ CURLcode Curl_auth_create_digest_md5_message(struct Curl_easy *data,
   for(i = 0; i < MD5_DIGEST_LEN; i++)
     curl_msnprintf(&resp_hash_hex[2 * i], 3, "%02x", digest[i]);
 
-  /* Generate the response */
-  response = curl_maprintf("username=\"%s\",realm=\"%s\",nonce=\"%s\","
-                           "cnonce=\"%s\",nc=\"%s\",digest-uri=\"%s\","
-                           "response=%s,qop=%s",
-                           userp, realm, nonce,
-                           cnonce, nonceCount, spn, resp_hash_hex, qop);
+  /* escape double quotes and backslashes in the realm and nonce as
+     necessary */
+  qrealm = auth_digest_string_quoted(realm);
+  qnonce = auth_digest_string_quoted(nonce);
+  if(qrealm && qnonce)
+    /* Generate the response */
+    response = curl_maprintf("username=\"%s\",realm=\"%s\",nonce=\"%s\","
+                             "cnonce=\"%s\",nc=\"%s\",digest-uri=\"%s\","
+                             "response=%s,qop=%s",
+                             userp, qrealm, qnonce,
+                             cnonce, nonceCount, spn, resp_hash_hex, qop);
+
+  curlx_free(qrealm);
+  curlx_free(qnonce);
   curlx_free(spn);
   if(!response)
     return CURLE_OUT_OF_MEMORY;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-081d2e73b7` → 本草稿移入 `cases/defect/auto-curl-081d2e73b7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
