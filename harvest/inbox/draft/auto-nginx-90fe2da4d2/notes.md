# auto-nginx-90fe2da4d2

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1561 (https://github.com/nginx/nginx/pull/1561)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 862；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -859,8 +859,8 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
 {
     off_t                         file_pos;
     u_char                        ch, sep, *pos, *lowcase_key;
-    size_t                        size, len, key_len, val_len, padding,
-                                  allocated;
+    size_t                        size, len, params_len,
+                                  key_len, val_len, padding, allocated;
     ngx_uint_t                    i, n, next, hash, skip_empty, header_params;
     ngx_buf_t                    *b;
     ngx_chain_t                  *cl, *body;
@@ -875,6 +875,7 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
     ngx_http_script_len_code_pt   lcode;
 
     len = 0;
+    params_len = 0;
     header_params = 0;
     ignored = NULL;
 
@@ -914,8 +915,10 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
                 continue;
             }
 
-            len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len;
+            params_len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len;
         }
+
+        len += params_len;
     }
 
     if (flcf->upstream.pass_request_headers) {
@@ -1071,6 +1074,7 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
 
         e.ip = params->values->elts;
         e.pos = b->last;
+        e.end = b->last + params_len;
         e.request = r;
         e.flushed = 1;
 
@@ -1103,6 +1107,12 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
                 continue;
             }
 
+            if (ngx_http_script_check_length(&e, 1 + ((val_len > 127) ? 4 : 1))
+                != NGX_OK)
+            {
+                return NGX_ERROR;
+            }
+
             *e.pos++ = (u_char) key_len;
 
             if (val_len > 127) {
@@ -1121,12 +1131,22 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
             }
             e.ip += sizeof(uintptr_t);
 
+            if (e.status) {
+                return NGX_ERROR;
+            }
+
             ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                            "fastcgi param: \"%*s: %*s\"",
                            key_len, e.pos - (key_len + val_len),
                            val_len, e.pos - val_len);
         }
 
+        if (e.pos != e.end) {
+            ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
+                          "fastcgi request length mismatch");
+            return NGX_ERROR;
+        }
+
         b->last = e.pos;
     }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-90fe2da4d2` → 本草稿移入 `cases/defect/auto-nginx-90fe2da4d2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
