# auto-nginx-996b48d67b

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1392 (https://github.com/nginx/nginx/pull/1392)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2301；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2298,11 +2298,10 @@ static ngx_int_t
 ngx_http_variable_request_id(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data)
 {
-    u_char  *id;
+    u_char    *id;
+    uint64_t   random_bytes[2];
 
-#if (NGX_OPENSSL)
-    u_char   random_bytes[16];
-#endif
+    static uint64_t  counter, key[2];
 
     id = ngx_pnalloc(r->pool, 32);
     if (id == NULL) {
@@ -2316,20 +2315,25 @@ ngx_http_variable_request_id(ngx_http_request_t *r,
     v->len = 32;
     v->data = id;
 
+    if (counter == 0) {
 #if (NGX_OPENSSL)
-
-    if (RAND_bytes(random_bytes, 16) == 1) {
-        ngx_hex_dump(id, random_bytes, 16);
-        return NGX_OK;
+        if (RAND_bytes((u_char *) key, 16) != 1)
+#endif
+        {
+            key[0] = ((uint64_t) ngx_random() << 32) | (uint32_t) ngx_random();
+            key[1] = ((uint64_t) ngx_random() << 32) | (uint32_t) ngx_random();
+            key[0] ^= (uint64_t) ngx_pid << 16;
+            key[1] ^= (uint64_t) ngx_time();
+        }
     }
 
-    ngx_ssl_error(NGX_LOG_ERR, r->connection->log, 0, "RAND_bytes() failed");
+    counter++;
+    random_bytes[0] = ngx_siphash(key[0], key[1], (u_char *) &counter, 8);
 
-#endif
+    counter++;
+    random_bytes[1] = ngx_siphash(key[0], key[1], (u_char *) &counter, 8);
 
-    ngx_sprintf(id, "%08xD%08xD%08xD%08xD",
-                (uint32_t) ngx_random(), (uint32_t) ngx_random(),
-                (uint32_t) ngx_random(), (uint32_t) ngx_random());
+    ngx_hex_dump(id, (u_char *) random_bytes, 16);
 
     return NGX_OK;
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-996b48d67b` → 本草稿移入 `cases/defect/auto-nginx-996b48d67b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
