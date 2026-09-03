# auto-nginx-f0198915d3

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1052 (https://github.com/nginx/nginx/pull/1052)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1497；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1494,8 +1494,9 @@ ngx_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len)
 uintptr_t
 ngx_escape_uri(u_char *dst, u_char *src, size_t size, ngx_uint_t type)
 {
-    ngx_uint_t      n;
+    u_char          prefix;
     uint32_t       *escape;
+    ngx_uint_t      n;
     static u_char   hex[] = "0123456789ABCDEF";
 
     /*
@@ -1633,11 +1634,36 @@ ngx_escape_uri(u_char *dst, u_char *src, size_t size, ngx_uint_t type)
 
                     /* mail_auth is the same as memcached */
 
+                    /* " ", "+", "=", not allowed */
+
+    static uint32_t   mail_xtext[] = {
+        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
+
+                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
+        0x20000801, /* 0010 0000 0000 0000  0000 1000 0000 0001 */
+
+                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
+        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
+
+                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
+        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
+
+        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
+        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
+        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
+        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
+    };
+
     static uint32_t  *map[] =
-        { uri, args, uri_component, html, refresh, memcached, memcached };
+        { uri, args, uri_component, html, refresh, memcached, memcached,
+          mail_xtext };
+
+    static u_char  map_char[] =
+        { '%', '%', '%', '%', '%', '%', '%', '+' };
 
 
     escape = map[type];
+    prefix = map_char[type];
 
     if (dst == NULL) {
 
@@ -1658,7 +1684,7 @@ ngx_escape_uri(u_char *dst, u_char *src, size_t size, ngx_uint_t type)
 
     while (size) {
         if (escape[*src >> 5] & (1U << (*src & 0x1f))) {
-            *dst++ = '%';
+            *dst++ = prefix;
             *dst++ = hex[*src >> 4];
             *dst++ = hex[*src & 0xf];
             src++;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-f0198915d3` → 本草稿移入 `cases/defect/auto-nginx-f0198915d3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
