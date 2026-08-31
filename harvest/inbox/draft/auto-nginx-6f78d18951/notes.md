# auto-nginx-6f78d18951

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #740 (https://github.com/nginx/nginx/pull/740)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 506；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -458,10 +458,18 @@ ngx_ssl_certificate(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *cert,
 {
     char            *err;
     X509            *x509, **elm;
+    u_long           n;
     EVP_PKEY        *pkey;
+    ngx_uint_t       mask;
     STACK_OF(X509)  *chain;
 
-    chain = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_CERT, &err, cert, NULL);
+    mask = 0;
+    elm = NULL;
+
+retry:
+
+    chain = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_CERT | mask,
+                                &err, cert, NULL);
     if (chain == NULL) {
         if (err != NULL) {
             ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
@@ -501,11 +509,16 @@ ngx_ssl_certificate(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *cert,
         }
     }
 
-    elm = ngx_array_push(&ssl->certs);
     if (elm == NULL) {
-        X509_free(x509);
-        sk_X509_pop_free(chain, X509_free);
-        return NGX_ERROR;
+        elm = ngx_array_push(&ssl->certs);
+        if (elm == NULL) {
+            X509_free(x509);
+            sk_X509_pop_free(chain, X509_free);
+            return NGX_ERROR;
+        }
+
+    } else {
+        X509_free(*elm);
     }
 
     *elm = x509;
@@ -528,11 +541,21 @@ ngx_ssl_certificate(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *cert,
     }
 
 #else
-    {
-    int  n;
 
     /* SSL_CTX_set0_chain() is only available in OpenSSL 1.0.2+ */
 
+#ifdef SSL_CTRL_CLEAR_EXTRA_CHAIN_CERTS
+    /* OpenSSL 1.0.1+ */
+    SSL_CTX_clear_extra_chain_certs(ssl->ctx);
+#else
+
+    if (ssl->ctx->extra_certs) {
+        sk_X509_pop_free(ssl->ctx->extra_certs, X509_free);
+        ssl->ctx->extra_certs = NULL;
+    }
+
+#endif
+
     n = sk_X509_num(chain);
 
     while (n--) {
@@ -548,10 +571,11 @@ ngx_ssl_certificate(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *cert,
     }
 
     sk_X509_free(chain);
-    }
+
 #endif
 
-    pkey = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_PKEY, &err, key, passwords);
+    pkey = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_PKEY | mask,
+                               &err, key, passwords);
     if (pkey == NULL) {
         if (err != NULL) {
             ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
@@ -563,9 +587,23 @@ ngx_ssl_certificate(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *cert,
     }
 
     if (SSL_CTX_use_PrivateKey(ssl->ctx, pkey) == 0) {
+        EVP_PKEY_free(pkey);
+
+        /* there can be mismatched pairs on uneven cache update */
+
+        n = ERR_peek_last_error();
+
+        if (ERR_GET_LIB(n) == ERR_LIB_X509
+            && ERR_GET_REASON(n) == X509_R_KEY_VALUES_MISMATCH
+            && mask == 0)
+        {
+            ERR_clear_error();
+            mask = NGX_SSL_CACHE_INVALIDATE;
+            goto retry;
+        }
+
         ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
                       "SSL_CTX_use_PrivateKey(\"%s\") failed", key->data);
-        EVP_PKEY_free(pkey);
         return NGX_ERROR;
     }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-6f78d18951` → 本草稿移入 `cases/defect/auto-nginx-6f78d18951/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
