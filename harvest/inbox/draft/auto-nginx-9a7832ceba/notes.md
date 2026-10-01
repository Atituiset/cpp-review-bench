# auto-nginx-9a7832ceba

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #840 (https://github.com/nginx/nginx/pull/840)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1653,6 +1653,102 @@ ngx_ssl_dhparam(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *file)
 }
 
 
+ngx_int_t
+ngx_ssl_ech_files(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_array_t *filenames)
+{
+#ifdef SSL_OP_ECH_GREASE
+    int             numkeys;
+    BIO            *in;
+    ngx_int_t       rc;
+    ngx_str_t      *filename;
+    ngx_uint_t      i;
+    OSSL_ECHSTORE  *es;
+
+    if (filenames == NULL) {
+        return NGX_OK;
+    }
+
+    es = OSSL_ECHSTORE_new(NULL, NULL);
+    if (es == NULL) {
+        ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0, "OSSL_ECHSTORE_new() failed");
+        return NGX_ERROR;
+    }
+
+    rc = NGX_ERROR;
+    filename = filenames->elts;
+
+    for (i = 0; i < filenames->nelts; i++) {
+
+        if (ngx_conf_full_name(cf->cycle, &filename[i], 1) != NGX_OK) {
+            goto cleanup;
+        }
+
+        in = BIO_new_file((char *) filename[i].data, "r");
+        if (in == NULL) {
+            ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
+                          "BIO_new_file(\"%s\") failed", filename[i].data);
+            goto cleanup;
+        }
+
+        /*
+         * We only set the ECHConfigList from the first file read to use
+         * in ECH retry-configs.
+         *
+         * That allows many sensible key rotation schemes so that the
+         * values sent in ECH retry-configs are smaller and current.
+         * For example, if the first file name has the current ECH
+         * private key, and a second one has the previously used key
+         * that some clients may still use due to DNS caching.
+         */
+
+         if (OSSL_ECHSTORE_read_pem(es, in, i ? OSSL_ECH_NO_RETRY
+                                              : OSSL_ECH_FOR_RETRY)
+             != 1)
+         {
+             ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
+                           "OSSL_ECHSTORE_read_pem(%s) failed",
+                           filename[i].data);
+             BIO_free(in);
+             goto cleanup;
+         }
+
+         BIO_free(in);
+    }
+
+    /*
+     * load the ECH store after checking there's at least one ECH
+     * private key in there (the PEM file spec allows zero or one
+     * private key per file)
+     */
+
+    if (OSSL_ECHSTORE_num_keys(es, &numkeys) != 1) {
+        ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
+                      "OSSL_ECHSTORE_num_keys(%s) failed");
+        goto cleanup;
+    }
+
+    if (numkeys > 0 && SSL_CTX_set1_echstore(ssl->ctx, es) != 1) {
+        ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
+                      "SSL_CTX_set1_echstore() failed");
+        goto cleanup;
+    }
+
+    rc = NGX_OK;
+
+cleanup:
+
+    OSSL_ECHSTORE_free(es);
+    return rc;
+
+#else
+    ngx_log_error(NGX_LOG_WARN, ssl->log, 0,
+                  "\"ssl_ech_file\" is not supported on this platform, "
+                  "ignored");
+    return NGX_OK;
+#endif
+}
+
+
 ngx_int_t
 ngx_ssl_ecdh_curve(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *name)
 {
@@ -5695,6 +5791,81 @@ ngx_ssl_get_alpn_protocol(ngx_connection_t *c, ngx_pool_t *pool, ngx_str_t *s)
 }
 
 
+ngx_int_t
+ngx_ssl_get_ech_status(ngx_connection_t *c, ngx_pool_t *pool, ngx_str_t *s)
+{
+#ifdef SSL_OP_ECH_GREASE
+    int    echrv;
+    char  *inner_sni, *outer_sni;
+
+    inner_sni = NULL;
+    outer_sni = NULL;
+
+    echrv = SSL_ech_get1_status(c->ssl->connection, &inner_sni, &outer_sni);
+
+    switch (echrv) {
+    case SSL_ECH_STATUS_NOT_TRIED:
+        ngx_str_set(s, "NOT_TRIED");
+        break;
+    case SSL_ECH_STATUS_SUCCESS:
+        ngx_str_set(s, "SUCCESS");
+        break;
+    case SSL_ECH_STATUS_GREASE:
+        ngx_str_set(s, "GREASE");
+        break;
+    case SSL_ECH_STATUS_BACKEND:
+        ngx_str_set(s, "BACKEND");
+        break;
+    default:
+        ngx_str_set(s, "FAILED");
+        break;
+    }
+
+    OPENSSL_free(inner_sni);
+    OPENSSL_free(outer_sni);
+#else
+    s->len = 0;
+#endif
+    return NGX_OK;
+}
+
+
+ngx_int_t
+ngx_ssl_get_ech_outer_server_name(ngx_connection_
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-9a7832ceba` → 本草稿移入 `cases/defect/auto-nginx-9a7832ceba/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
