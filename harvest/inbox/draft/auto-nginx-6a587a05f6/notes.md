# auto-nginx-6a587a05f6

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1475 (https://github.com/nginx/nginx/pull/1475)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -749,6 +749,12 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
         tmp_len = 0;
 
     } else {
+        if (r->method_name.len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 method: \"%V\"", &r->method_name);
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + r->method_name.len;
         tmp_len = r->method_name.len;
     }
@@ -769,6 +775,12 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
         uri_len = r->uri.len + escape + sizeof("?") - 1 + r->args.len;
     }
 
+    if (uri_len > NGX_HTTP_V2_MAX_FIELD) {
+        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                      "too long http2 URI");
+        return NGX_ERROR;
+    }
+
     len += 1 + NGX_HTTP_V2_INT_OCTETS + uri_len;
 
     if (tmp_len < uri_len) {
@@ -778,6 +790,12 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
     /* :authority header */
 
     if (!glcf->host_set) {
+        if (ctx->host.len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                          "too long http2 host: \"%V\"", &ctx->host);
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + ctx->host.len;
 
         if (tmp_len < ctx->host.len) {
@@ -808,6 +826,18 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
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
 
@@ -842,6 +872,20 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
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
1. 评论 `/case accept auto-nginx-6a587a05f6` → 本草稿移入 `cases/defect/auto-nginx-6a587a05f6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
