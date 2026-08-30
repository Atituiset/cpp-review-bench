# auto-nginx-e59dbbbc18

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1433 (https://github.com/nginx/nginx/pull/1433)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 104；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -101,10 +101,11 @@ static ngx_int_t
 ngx_http_secure_link_variable(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data)
 {
-    u_char                       *p, *last;
-    ngx_str_t                     val, hash;
+    u_char                       *p, *last, ch;
     time_t                        expires;
+    ngx_str_t                     val, hash;
     ngx_md5_t                     md5;
+    ngx_uint_t                    i;
     ngx_http_secure_link_ctx_t   *ctx;
     ngx_http_secure_link_conf_t  *conf;
     u_char                        hash_buf[18], md5_buf[16];
@@ -175,7 +176,13 @@ ngx_http_secure_link_variable(ngx_http_request_t *r,
     ngx_md5_update(&md5, val.data, val.len);
     ngx_md5_final(md5_buf, &md5);
 
-    if (ngx_memcmp(hash_buf, md5_buf, 16) != 0) {
+    /* constant time comparison */
+
+    for (ch = 0, i = 0; i < 16; i++) {
+        ch |= (hash_buf[i] ^ md5_buf[i]);
+    }
+
+    if (ch) {
         goto not_found;
     }
 
@@ -200,11 +207,11 @@ ngx_http_secure_link_old_variable(ngx_http_request_t *r,
     ngx_http_secure_link_conf_t *conf, ngx_http_variable_value_t *v,
     uintptr_t data)
 {
-    u_char      *p, *start, *end, *last;
+    u_char      *p, *start, *end, *last, ch;
     size_t       len;
     ngx_int_t    n;
-    ngx_uint_t   i;
     ngx_md5_t    md5;
+    ngx_uint_t   i;
     u_char       hash[16];
 
     p = &r->unparsed_uri.data[1];
@@ -243,11 +250,19 @@ ngx_http_secure_link_old_variable(ngx_http_request_t *r,
     ngx_md5_update(&md5, conf->secret.data, conf->secret.len);
     ngx_md5_final(hash, &md5);
 
-    for (i = 0; i < 16; i++) {
+    for (ch = 0, i = 0; i < 16; i++) {
         n = ngx_hextoi(&start[2 * i], 2);
-        if (n == NGX_ERROR || n != hash[i]) {
+        if (n == NGX_ERROR) {
             goto not_found;
         }
+
+        /* constant time comparison */
+
+        ch |= (u_char) n ^ hash[i];
+    }
+
+    if (ch) {
+        goto not_found;
     }
 
     v->len = len;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-e59dbbbc18` → 本草稿移入 `cases/defect/auto-nginx-e59dbbbc18/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
