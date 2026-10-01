# auto-nginx-7b0fc4cb22

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #889 (https://github.com/nginx/nginx/pull/889)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 698；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -695,30 +695,35 @@ ngx_quic_handshake(ngx_connection_t *c)
 
     ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0, "SSL_do_handshake: %d", n);
 
-    if (qc->error) {
-        return NGX_ERROR;
-    }
-
     if (n <= 0) {
         sslerr = SSL_get_error(ssl_conn, n);
 
         ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0, "SSL_get_error: %d",
                        sslerr);
 
-        if (sslerr != SSL_ERROR_WANT_READ) {
-
-            if (c->ssl->handshake_rejected) {
-                ngx_connection_error(c, 0, "handshake rejected");
-                ERR_clear_error();
+        if (c->ssl->handshake_rejected) {
+            ngx_connection_error(c, 0, "handshake rejected");
+            ERR_clear_error();
+            return NGX_ERROR;
+        }
 
-                return NGX_ERROR;
-            }
+        if (qc->error) {
+            ngx_connection_error(c, 0, "SSL_do_handshake() failed");
+            ERR_clear_error();
+            return NGX_ERROR;
+        }
 
+        if (sslerr != SSL_ERROR_WANT_READ) {
             ngx_ssl_connection_error(c, sslerr, 0, "SSL_do_handshake() failed");
             return NGX_ERROR;
         }
     }
 
+    if (qc->error) {
+        ngx_connection_error(c, 0, "SSL_do_handshake() failed");
+        return NGX_ERROR;
+    }
+
     if (!SSL_is_init_finished(ssl_conn)) {
         if (ngx_quic_keys_available(qc->keys, NGX_QUIC_ENCRYPTION_EARLY_DATA, 0)
             && qc->client_tp_done)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-7b0fc4cb22` → 本草稿移入 `cases/defect/auto-nginx-7b0fc4cb22/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
