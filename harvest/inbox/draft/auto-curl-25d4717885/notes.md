# auto-curl-25d4717885

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #961c95fea6e097ef4dbc19ee996e5127e58acbac (https://github.com/curl/curl/commit/961c95fea6e097ef4dbc19ee996e5127e58acbac)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -4669,6 +4669,7 @@ static CURLcode ossl_infof_cert(struct Curl_cfilter *cf,
 struct ossl_certs_ctx {
   STACK_OF(X509) *sk;
   size_t num_certs;
+  unsigned char *last_der;
 };
 
 static CURLcode ossl_chain_get_der(struct Curl_cfilter *cf,
@@ -4682,6 +4683,9 @@ static CURLcode ossl_chain_get_der(struct Curl_cfilter *cf,
   X509 *cert;
   int der_len;
 
+  OPENSSL_free(chain->last_der);
+  chain->last_der = NULL;
+
   (void)cf;
   (void)data;
   *pder_len = 0;
@@ -4695,6 +4699,7 @@ static CURLcode ossl_chain_get_der(struct Curl_cfilter *cf,
   der_len = i2d_X509(cert, pder);
   if(der_len < 0)
     return CURLE_FAILED_INIT;
+  chain->last_der = *pder;
   *pder_len = (size_t)der_len;
   return CURLE_OK;
 }
@@ -4748,6 +4753,8 @@ static CURLcode ossl_apple_verify(struct Curl_cfilter *cf,
     result = Curl_vtls_apple_verify(cf, data, peer, chain.num_certs,
                                     ossl_chain_get_der, &chain,
                                     ocsp_data, ocsp_len);
+    OPENSSL_free(chain.last_der);
+    chain.last_der = NULL;
     if(!result && ocsp_missing && conn_config->verifystatus &&
        !octx->reused_session) {
       /* verified, but OCSP stapling is required and server sent none */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-25d4717885` → 本草稿移入 `cases/defect/auto-curl-25d4717885/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
