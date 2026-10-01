# auto-nginx-016f27b9fe

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #842 (https://github.com/nginx/nginx/pull/842)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 594；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -523,7 +523,7 @@ ngx_mail_starttls_only(ngx_mail_session_t *s, ngx_connection_t *c)
 ngx_int_t
 ngx_mail_auth_plain(ngx_mail_session_t *s, ngx_connection_t *c, ngx_uint_t n)
 {
-    u_char     *p, *last;
+    u_char     *p, *pos, *last;
     ngx_str_t  *arg, plain;
 
     arg = s->args.elts;
@@ -555,7 +555,7 @@ ngx_mail_auth_plain(ngx_mail_session_t *s, ngx_connection_t *c, ngx_uint_t n)
         return NGX_MAIL_PARSE_INVALID_COMMAND;
     }
 
-    s->login.data = p;
+    pos = p;
 
     while (p < last && *p) { p++; }
 
@@ -565,7 +565,8 @@ ngx_mail_auth_plain(ngx_mail_session_t *s, ngx_connection_t *c, ngx_uint_t n)
         return NGX_MAIL_PARSE_INVALID_COMMAND;
     }
 
-    s->login.len = p++ - s->login.data;
+    s->login.len = p++ - pos;
+    s->login.data = pos;
 
     s->passwd.len = last - p;
     s->passwd.data = p;
@@ -583,24 +584,26 @@ ngx_int_t
 ngx_mail_auth_login_username(ngx_mail_session_t *s, ngx_connection_t *c,
     ngx_uint_t n)
 {
-    ngx_str_t  *arg;
+    ngx_str_t  *arg, login;
 
     arg = s->args.elts;
 
     ngx_log_debug1(NGX_LOG_DEBUG_MAIL, c->log, 0,
                    "mail auth login username: \"%V\"", &arg[n]);
 
-    s->login.data = ngx_pnalloc(c->pool, ngx_base64_decoded_length(arg[n].len));
-    if (s->login.data == NULL) {
+    login.data = ngx_pnalloc(c->pool, ngx_base64_decoded_length(arg[n].len));
+    if (login.data == NULL) {
         return NGX_ERROR;
     }
 
-    if (ngx_decode_base64(&s->login, &arg[n]) != NGX_OK) {
+    if (ngx_decode_base64(&login, &arg[n]) != NGX_OK) {
         ngx_log_error(NGX_LOG_INFO, c->log, 0,
             "client sent invalid base64 encoding in AUTH LOGIN command");
         return NGX_MAIL_PARSE_INVALID_COMMAND;
     }
 
+    s->login = login;
+
     ngx_log_debug1(NGX_LOG_DEBUG_MAIL, c->log, 0,
                    "mail auth login username: \"%V\"", &s->login);
 
@@ -611,7 +614,7 @@ ngx_mail_auth_login_username(ngx_mail_session_t *s, ngx_connection_t *c,
 ngx_int_t
 ngx_mail_auth_login_password(ngx_mail_session_t *s, ngx_connection_t *c)
 {
-    ngx_str_t  *arg;
+    ngx_str_t  *arg, passwd;
 
     arg = s->args.elts;
 
@@ -620,18 +623,19 @@ ngx_mail_auth_login_password(ngx_mail_session_t *s, ngx_connection_t *c)
                    "mail auth login password: \"%V\"", &arg[0]);
 #endif
 
-    s->passwd.data = ngx_pnalloc(c->pool,
-                                 ngx_base64_decoded_length(arg[0].len));
-    if (s->passwd.data == NULL) {
+    passwd.data = ngx_pnalloc(c->pool, ngx_base64_decoded_length(arg[0].len));
+    if (passwd.data == NULL) {
         return NGX_ERROR;
     }
 
-    if (ngx_decode_base64(&s->passwd, &arg[0]) != NGX_OK) {
+    if (ngx_decode_base64(&passwd, &arg[0]) != NGX_OK) {
         ngx_log_error(NGX_LOG_INFO, c->log, 0,
             "client sent invalid base64 encoding in AUTH LOGIN command");
         return NGX_MAIL_PARSE_INVALID_COMMAND;
     }
 
+    s->passwd = passwd;
+
 #if (NGX_DEBUG_MAIL_PASSWD)
     ngx_log_debug1(NGX_LOG_DEBUG_MAIL, c->log, 0,
                    "mail auth login password: \"%V\"", &s->passwd);
@@ -674,24 +678,26 @@ ngx_int_t
 ngx_mail_auth_cram_md5(ngx_mail_session_t *s, ngx_connection_t *c)
 {
     u_char     *p, *last;
-    ngx_str_t  *arg;
+    ngx_str_t  *arg, login;
 
     arg = s->args.elts;
 
     ngx_log_debug1(NGX_LOG_DEBUG_MAIL, c->log, 0,
                    "mail auth cram-md5: \"%V\"", &arg[0]);
 
-    s->login.data = ngx_pnalloc(c->pool, ngx_base64_decoded_length(arg[0].len));
-    if (s->login.data == NULL) {
+    login.data = ngx_pnalloc(c->pool, ngx_base64_decoded_length(arg[0].len));
+    if (login.data == NULL) {
         return NGX_ERROR;
     }
 
-    if (ngx_decode_base64(&s->login, &arg[0]) != NGX_OK) {
+    if (ngx_decode_base64(&login, &arg[0]) != NGX_OK) {
         ngx_log_error(NGX_LOG_INFO, c->log, 0,
             "client sent invalid base64 encoding in AUTH CRAM-MD5 command");
         return NGX_MAIL_PARSE_INVALID_COMMAND;
     }
 
+    s->login = login;
+
     p = 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-016f27b9fe` → 本草稿移入 `cases/defect/auto-nginx-016f27b9fe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
