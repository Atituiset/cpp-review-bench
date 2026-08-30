# auto-nginx-748db57cb5

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #775 (https://github.com/nginx/nginx/pull/775)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1377；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1374,7 +1374,7 @@ ngx_ssl_dhparam(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *file)
     if (SSL_CTX_set0_tmp_dh_pkey(ssl->ctx, dh) != 1) {
         ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
                       "SSL_CTX_set0_tmp_dh_pkey(\"%s\") failed", file->data);
-#if (OPENSSL_VERSION_NUMBER >= 0x3000001fL)
+#if (OPENSSL_VERSION_NUMBER >= 0x30000010L)
         EVP_PKEY_free(dh);
 #endif
         BIO_free(bio);
@@ -5055,11 +5055,7 @@ ngx_ssl_get_curve(ngx_connection_t *c, ngx_pool_t *pool, ngx_str_t *s)
             return NGX_OK;
         }
 
-#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
         name = SSL_group_to_name(c->ssl->connection, nid);
-#else
-        name = NULL;
-#endif
 
         s->len = name ? ngx_strlen(name) : sizeof("0x0000") - 1;
         s->data = ngx_pnalloc(pool, s->len);
@@ -5113,11 +5109,7 @@ ngx_ssl_get_curves(ngx_connection_t *c, ngx_pool_t *pool, ngx_str_t *s)
         nid = curves[i];
 
         if (nid & TLSEXT_nid_unknown) {
-#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
             name = SSL_group_to_name(c->ssl->connection, nid);
-#else
-            name = NULL;
-#endif
 
             len += name ? ngx_strlen(name) : sizeof("0x0000") - 1;
 
@@ -5139,11 +5131,7 @@ ngx_ssl_get_curves(ngx_connection_t *c, ngx_pool_t *pool, ngx_str_t *s)
         nid = curves[i];
 
         if (nid & TLSEXT_nid_unknown) {
-#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
             name = SSL_group_to_name(c->ssl->connection, nid);
-#else
-            name = NULL;
-#endif
 
             p = name ? ngx_cpymem(p, name, ngx_strlen(name))
                      : ngx_sprintf(p, "0x%04xd", nid & 0xffff);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-748db57cb5` → 本草稿移入 `cases/defect/auto-nginx-748db57cb5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
