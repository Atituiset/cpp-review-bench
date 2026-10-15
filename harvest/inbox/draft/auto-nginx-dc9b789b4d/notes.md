# auto-nginx-dc9b789b4d

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1474 (https://github.com/nginx/nginx/pull/1474)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -379,6 +379,12 @@ ngx_http_proxy_v2_create_request(ngx_http_request_t *r)
         tmp_len = 0;
 
     } else {
+        if (method.len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 method: \"%V\"", &method);
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + method.len;
         tmp_len = method.len;
     }
@@ -419,6 +425,12 @@ ngx_http_proxy_v2_create_request(ngx_http_request_t *r)
         return NGX_ERROR;
     }
 
+    if (uri_len > NGX_HTTP_V2_MAX_FIELD) {
+        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                      "too long http2 URI");
+        return NGX_ERROR;
+    }
+
     len += 1 + NGX_HTTP_V2_INT_OCTETS + uri_len;
 
     if (tmp_len < uri_len) {
@@ -430,6 +442,12 @@ ngx_http_proxy_v2_create_request(ngx_http_request_t *r)
     host = &ctx->ctx.vars.host_header;
 
     if (!plcf->host_set) {
+        if (host->len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 host: \"%V\"", host);
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + host->len;
 
         if (tmp_len < host->len) {
@@ -483,6 +501,18 @@ ngx_http_proxy_v2_create_request(ngx_http_request_t *r)
             continue;
         }
 
+        if (key_len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 header name");
+            return NGX_ERROR;
+        }
+
+        if (val_len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 header value");
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + key_len
                  + NGX_HTTP_V2_INT_OCTETS + val_len;
 
@@ -517,6 +547,20 @@ ngx_http_proxy_v2_create_request(ngx_http_request_t *r)
                 continue;
             }
 
+            if (header[i].key.len > NGX_HTTP_V2_MAX_FIELD) {
+                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                              "too long http2 header name: \"%V\"",
+                              &header[i].key);
+                return NGX_ERROR;
+            }
+
+            if (header[i].value.len > NGX_HTTP_V2_MAX_FIELD) {
+                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                              "too long http2 header value: \"%V: %V\"",
+                              &header[i].key, &header[i].value);
+                return NGX_ERROR;
+            }
+
             len += 1 + NGX_HTTP_V2_INT_OCTETS + header[i].key.len
                      + NGX_HTTP_V2_INT_OCTETS + header[i].value.len;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-dc9b789b4d` → 本草稿移入 `cases/defect/auto-nginx-dc9b789b4d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
