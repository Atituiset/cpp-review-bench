# auto-nginx-7dad90af1d

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1109 (https://github.com/nginx/nginx/pull/1109)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -42,6 +42,7 @@ typedef struct {
     ngx_str_t                        ssl_ciphers;
     ngx_stream_complex_value_t      *ssl_name;
     ngx_flag_t                       ssl_server_name;
+    ngx_array_t                     *ssl_alpn;
 
     ngx_flag_t                       ssl_verify;
     ngx_uint_t                       ssl_verify_depth;
@@ -95,6 +96,8 @@ static char *ngx_stream_proxy_bind(ngx_conf_t *cf, ngx_command_t *cmd,
 #if (NGX_STREAM_SSL)
 
 static ngx_int_t ngx_stream_proxy_send_proxy_protocol(ngx_stream_session_t *s);
+static char *ngx_stream_proxy_ssl_alpn_set_slot(ngx_conf_t *cf,
+    ngx_command_t *cmd, void *conf);
 static char *ngx_stream_proxy_ssl_certificate_cache(ngx_conf_t *cf,
     ngx_command_t *cmd, void *conf);
 static char *ngx_stream_proxy_ssl_password_file(ngx_conf_t *cf,
@@ -105,6 +108,7 @@ static void ngx_stream_proxy_ssl_init_connection(ngx_stream_session_t *s);
 static void ngx_stream_proxy_ssl_handshake(ngx_connection_t *pc);
 static void ngx_stream_proxy_ssl_save_session(ngx_connection_t *c);
 static ngx_int_t ngx_stream_proxy_ssl_name(ngx_stream_session_t *s);
+static ngx_int_t ngx_stream_proxy_ssl_alpn(ngx_stream_session_t *s);
 static ngx_int_t ngx_stream_proxy_ssl_certificate(ngx_stream_session_t *s);
 static ngx_int_t ngx_stream_proxy_merge_ssl(ngx_conf_t *cf,
     ngx_stream_proxy_srv_conf_t *conf, ngx_stream_proxy_srv_conf_t *prev);
@@ -304,6 +308,13 @@ static ngx_command_t  ngx_stream_proxy_commands[] = {
       offsetof(ngx_stream_proxy_srv_conf_t, ssl_server_name),
       NULL },
 
+    { ngx_string("proxy_ssl_alpn"),
+      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_1MORE,
+      ngx_stream_proxy_ssl_alpn_set_slot,
+      NGX_STREAM_SRV_CONF_OFFSET,
+      0,
+      NULL },
+
     { ngx_string("proxy_ssl_verify"),
       NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
       ngx_conf_set_flag_slot,
@@ -1043,6 +1054,61 @@ ngx_stream_proxy_send_proxy_protocol(ngx_stream_session_t *s)
 }
 
 
+static char *
+ngx_stream_proxy_ssl_alpn_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
+    void *conf)
+{
+#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
+
+    ngx_stream_proxy_srv_conf_t *pscf = conf;
+
+    ngx_str_t                           *value;
+    ngx_uint_t                           i;
+    ngx_stream_complex_value_t          *cv;
+    ngx_stream_compile_complex_value_t   ccv;
+
+    if (pscf->ssl_alpn != NGX_CONF_UNSET_PTR) {
+        return "is duplicate";
+    }
+
+    value = cf->args->elts;
+
+    pscf->ssl_alpn = ngx_array_create(cf->pool, cf->args->nelts - 1,
+                                      sizeof(ngx_stream_complex_value_t));
+    if (pscf->ssl_alpn == NULL) {
+        return NGX_CONF_ERROR;
+    }
+
+    cv = ngx_array_push_n(pscf->ssl_alpn, cf->args->nelts - 1);
+
+    for (i = 1; i < cf->args->nelts; i++) {
+        ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));
+
+        ccv.cf = cf;
+        ccv.value = &value[i];
+        ccv.complex_value = &cv[i - 1];
+
+        if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
+            return NGX_CONF_ERROR;
+        }
+
+        if (cv[i - 1].lengths == NULL && value[i].len > 255) {
+            return "protocol too long";
+        }
+    }
+
+    return NGX_CONF_OK;
+
+#else
+    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
+                       "the \"proxy_ssl_alpn\" directive requires "
+                       "OpenSSL with ALPN support");
+
+    return NGX_CONF_ERROR;
+#endif
+}
+
+
 static char *
 ngx_stream_proxy_ssl_certificate_cache(ngx_conf_t *cf, ngx_command_t *cmd,
     void *conf)
@@ -1200,6 +1266,13 @@ ngx_stream_proxy_ssl_init_connection(ngx_stream_session_t *s)
         }
     }
 
+    if (pscf->ssl_alpn) {
+        if (ngx_stream_proxy_ssl_alpn(s) != NGX_OK) {
+            ngx_stream_proxy_finalize(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
+            return;
+        }
+    }
+
     if (pscf->ssl_certificate
         && pscf->ssl_certificate->value.len
       
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-7dad90af1d` → 本草稿移入 `cases/defect/auto-nginx-7dad90af1d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
