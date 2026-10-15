# auto-nginx-fbfa2f09e6

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #740 (https://github.com/nginx/nginx/pull/740)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 239；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -193,6 +193,7 @@ ngx_ssl_cache_fetch(ngx_conf_t *cf, ngx_uint_t index, char **err,
     time_t                 mtime;
     uint32_t               hash;
     ngx_int_t              rc;
+    ngx_uint_t             invalidate;
     ngx_file_uniq_t        uniq;
     ngx_file_info_t        fi;
     ngx_ssl_cache_t       *cache, *old_cache;
@@ -202,10 +203,17 @@ ngx_ssl_cache_fetch(ngx_conf_t *cf, ngx_uint_t index, char **err,
 
     *err = NULL;
 
+    invalidate = index & NGX_SSL_CACHE_INVALIDATE;
+    index &= ~NGX_SSL_CACHE_INVALIDATE;
+
     if (ngx_ssl_cache_init_key(cf->pool, index, path, &id) != NGX_OK) {
         return NULL;
     }
 
+    if (id.type == NGX_SSL_CACHE_DATA) {
+        invalidate = 0;
+    }
+
     cache = (ngx_ssl_cache_t *) ngx_get_conf(cf->cycle->conf_ctx,
                                              ngx_openssl_cache_module);
 
@@ -215,7 +223,12 @@ ngx_ssl_cache_fetch(ngx_conf_t *cf, ngx_uint_t index, char **err,
     cn = ngx_ssl_cache_lookup(cache, type, &id, hash);
 
     if (cn != NULL) {
-        return type->ref(err, cn->value);
+        if (!invalidate) {
+            return type->ref(err, cn->value);
+        }
+
+        type->free(cn->value);
+        ngx_rbtree_delete(&cache->rbtree, &cn->node);
     }
 
     value = NULL;
@@ -236,7 +249,7 @@ ngx_ssl_cache_fetch(ngx_conf_t *cf, ngx_uint_t index, char **err,
 
     old_cache = ngx_ssl_cache_get_old_conf(cf->cycle);
 
-    if (old_cache && old_cache->inheritable) {
+    if (old_cache && old_cache->inheritable && !invalidate) {
         cn = ngx_ssl_cache_lookup(old_cache, type, &id, hash);
 
         if (cn != NULL) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-fbfa2f09e6` → 本草稿移入 `cases/defect/auto-nginx-fbfa2f09e6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
