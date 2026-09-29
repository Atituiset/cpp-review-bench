# auto-nginx-a937d45053

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1633 (https://github.com/nginx/nginx/pull/1633)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 12（原始 PR diff 行 870；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -671,8 +671,8 @@ static ngx_int_t
 ngx_http_init_locations(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
     ngx_http_core_loc_conf_t *pclcf)
 {
-    ngx_uint_t                   n;
-    ngx_queue_t                 *q, *locations, *named, tail;
+    ngx_uint_t                   n, p;
+    ngx_queue_t                 *q, *locations, *named, *predicate, tail;
     ngx_http_core_loc_conf_t    *clcf;
     ngx_http_location_queue_t   *lq;
     ngx_http_core_loc_conf_t   **clcfp;
@@ -695,6 +695,8 @@ ngx_http_init_locations(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
     regex = NULL;
     r = 0;
 #endif
+    predicate = NULL;
+    p = 0;
 
     for (q = ngx_queue_head(locations);
          q != ngx_queue_sentinel(locations);
@@ -722,6 +724,16 @@ ngx_http_init_locations(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
 
 #endif
 
+        if (clcf->predicate) {
+            p++;
+
+            if (predicate == NULL) {
+                predicate = q;
+            }
+
+            continue;
+        }
+
         if (clcf->named) {
             n++;
 
@@ -764,6 +776,29 @@ ngx_http_init_locations(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
         ngx_queue_split(locations, named, &tail);
     }
 
+    if (predicate) {
+        clcfp = ngx_palloc(cf->pool,
+                           (p + 1) * sizeof(ngx_http_core_loc_conf_t *));
+        if (clcfp == NULL) {
+            return NGX_ERROR;
+        }
+
+        pclcf->predicate_locations = clcfp;
+
+        for (q = predicate;
+             q != ngx_queue_sentinel(locations);
+             q = ngx_queue_next(q))
+        {
+            lq = (ngx_http_location_queue_t *) q;
+
+            *(clcfp++) = lq->exact;
+        }
+
+        *clcfp = NULL;
+
+        ngx_queue_split(locations, predicate, &tail);
+    }
+
 #if (NGX_PCRE)
 
     if (regex) {
@@ -801,9 +836,17 @@ ngx_http_init_static_location_trees(ngx_conf_t *cf,
     ngx_http_core_loc_conf_t *pclcf)
 {
     ngx_queue_t                *q, *locations;
-    ngx_http_core_loc_conf_t   *clcf;
+    ngx_http_core_loc_conf_t   *clcf, **clcfp;
     ngx_http_location_queue_t  *lq;
 
+    if (pclcf->predicate_locations) {
+        for (clcfp = pclcf->predicate_locations; *clcfp; clcfp++) {
+            if (ngx_http_init_static_location_trees(cf, *clcfp) != NGX_OK) {
+                return NGX_ERROR;
+            }
+        }
+    }
+
     locations = pclcf->locations;
 
     if (locations == NULL) {
@@ -867,7 +910,7 @@ ngx_http_add_location(ngx_conf_t *cf, ngx_queue_t **locations,
 #if (NGX_PCRE)
         || clcf->regex
 #endif
-        || clcf->named || clcf->noname)
+        || clcf->predicate || clcf->named || clcf->noname)
     {
         lq->exact = clcf;
         lq->inclusive = NULL;
@@ -966,6 +1009,21 @@ ngx_http_cmp_locations(const ngx_queue_t *one, const ngx_queue_t *two)
         return ngx_strcmp(first->name.data, second->name.data);
     }
 
+    if (first->predicate && !second->predicate) {
+        /* shift predicate locations to the end */
+        return 1;
+    }
+
+    if (!first->predicate && second->predicate) {
+        /* shift predicate locations to the end */
+        return -1;
+    }
+
+    if (first->predicate || second->predicate) {
+        /* do not sort the predicate locations */
+        return 0;
+    }
+
 #if (NGX_PCRE)
 
     if (first->regex && !second->regex) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-a937d45053` → 本草稿移入 `cases/defect/auto-nginx-a937d45053/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
